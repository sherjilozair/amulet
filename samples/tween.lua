local am = amulet

local vshader = [[
    attribute float x;
    attribute float y;
    uniform mat4 MVP;
    void main() {
        gl_Position = MVP * vec4(x, y, 0, 1);
    }
]]

local fshader = [[
    precision mediump float;
    uniform vec4 tint;
    void main() {
        gl_FragColor = tint;
    }
]]

local win = am.window({title = "tween test"})

local prog = am.program(vshader, fshader)

local buf = am.buffer(4 * 6)
local xview = buf:view("float", 0, 8)
local yview = buf:view("float", 4, 8)
xview[1] = -0.05
xview[2] = 0
xview[3] = 0.05
yview[1] = -0.05
yview[2] = 0.05
yview[3] = -0.05

local MVP = math.mat4(1)
local base = am.draw_arrays()
    :bind_array("x", xview)
    :bind_array("y", yview)

local group = am.group()

local num_tris = 100
local seeds = {}
for i = 1, num_tris do
    table.insert(seeds, math.random())
end

local e = am.ease

local easings = {
    e.linear,
    e.quadratic,
    e.out(e.quadratic),
    e.cubic,
    e.out(e.cubic),
    e.inout(e.cubic, e.quadratic),
    e.inout(e.cubic),
    e.hyperbola,
    e.inout(e.hyperbola),
    e.sine,
    e.windup,
    e.out(e.windup),
    e.bounce,
    e.elastic,
    e.cubic_bezier(vec2(0.1, 0.4), vec2(0.6, 0.9)),
}
local group = am.group()
local
function wait(f)
    local t = type(f)
    if t == "function" then
        local r = f()
        while r do
            coroutine.yield(r)
            r = f()
        end
    elseif t == "number" then
        coroutine.yield(f)
    else
        error("unexpected wait function argument: "..tostring(f), 2)
    end
end

for i, easing in ipairs(easings) do
    local y = #easings == 1 and 0 or - (i - 1) / (#easings - 1) * 1.6 + 0.8
    local node = base:scale("MVP"):translate("MVP", -0.5, y)
        :action(coroutine.create(function(node)
            local target_x = 0.5
            ::start::
                wait(am.tween{target = node, x = target_x, time = 1, ease = easings[i]})
                wait(0.3)
                target_x = target_x > 0 and -0.5 or 0.5
            goto start
        end))
        :bind_vec4("tint", 1, 1, 1, 1)
    group:append(node)
end

local top = group:bind_mat4("MVP", MVP):bind_program(prog)

win.root = top
top:action(function()
    if win:key_pressed("escape") then
        win:close()
    end
    return 0
end)