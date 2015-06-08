#include "amulet.h"

am_scene_node::am_scene_node() {
    children.owner = this;
    recursion_limit = am_conf_default_recursion_limit;
    flags = 0;
    actions_ref = LUA_NOREF;
    action_seq = 0;
}

void am_scene_node::render_children(am_render_state *rstate) {
    if (recursion_limit < 0) return;
    recursion_limit--;
    for (int i = 0; i < children.size; i++) {
        am_scene_node *child = children.arr[i].child;
        child->render(rstate);
    }
    recursion_limit++;
}

int am_scene_node::specialized_index(lua_State *L) {
    return 0;
}

int am_scene_node::specialized_newindex(lua_State *L) {
    return -1;
}

void am_scene_node::render(am_render_state *rstate) {
    render_children(rstate);
}

static int search_uservalues(lua_State *L, am_scene_node *node) {
    if (am_node_marked(node)) return 0; // cycle
    node->pushuservalue(L); // push uservalue table of node
    lua_pushvalue(L, 2); // push field
    lua_rawget(L, -2); // lookup field in uservalue table
    if (!lua_isnil(L, -1)) {
        // found it
        lua_remove(L, -2); // remove uservalue table
        return 1;
    }
    lua_pop(L, 2); // pop nil, uservalue table
    if (node->children.size != 1) return 0;
    am_mark_node(node);
    am_scene_node *child = node->children.arr[0].child;
    child->push(L);
    lua_replace(L, 1); // child is now at index 1
    int r = search_uservalues(L, child);
    am_unmark_node(node);
    return r;
}

int am_scene_node_index(lua_State *L) {
    am_scene_node *node = (am_scene_node*)lua_touserdata(L, 1);
    if (node->specialized_index(L)) return 1;
    am_default_index_func(L); // check metatable
    if (!lua_isnil(L, -1)) return 1;
    lua_pop(L, 1); // pop nil
    return search_uservalues(L, (am_scene_node*)lua_touserdata(L, 1));
}

int am_scene_node_newindex(lua_State *L) {
    am_scene_node *node = (am_scene_node*)lua_touserdata(L, 1);
    if (node->specialized_newindex(L) != -1) return 0;
    return am_default_newindex_func(L);
}

static int append_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *child = am_get_userdata(L, am_scene_node, 2);
    am_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 2); // ref from parent to child
    parent->children.push_back(L, child_slot);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int prepend_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *child = am_get_userdata(L, am_scene_node, 2);
    am_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 2); // ref from parent to child
    parent->children.push_front(L, child_slot);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *child = am_get_userdata(L, am_scene_node, 2);
    for (int i = 0; i < parent->children.size; i++) {
        if (parent->children.arr[i].child == child) {
            parent->unref(L, parent->children.arr[i].ref);
            parent->children.remove(i);
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int replace_child(lua_State *L) {
    am_check_nargs(L, 3);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    am_scene_node *old_child = am_get_userdata(L, am_scene_node, 2);
    am_scene_node *new_child = am_get_userdata(L, am_scene_node, 3);
    for (int i = 0; i < parent->children.size; i++) {
        if (parent->children.arr[i].child == old_child) {
            parent->unref(L, parent->children.arr[i].ref);
            parent->children.remove(i);
            am_node_child slot;
            slot.child = new_child;
            slot.ref = parent->ref(L, 3);
            parent->children.insert(L, i, slot);
            break;
        }
    }
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

static int remove_all_children(lua_State *L) {
    am_check_nargs(L, 1);
    am_scene_node *parent = am_get_userdata(L, am_scene_node, 1);
    for (int i = parent->children.size-1; i >= 0; i--) {
        parent->unref(L, parent->children.arr[i].ref);
        parent->children.remove(i);
    }
    assert(parent->children.size == 0);
    lua_pushvalue(L, 1); // for chaining
    return 1;
}

void am_set_scene_node_child(lua_State *L, am_scene_node *parent) {
    if (lua_isnil(L, 1)) {
        return;
    }
    am_scene_node *child = am_get_userdata(L, am_scene_node, 1);
    am_node_child child_slot;
    child_slot.child = child;
    child_slot.ref = parent->ref(L, 1); // ref from parent to child
    parent->children.push_back(L, child_slot);
}

static int create_wrap_node(lua_State *L) {
    int nargs = am_check_nargs(L, 0);
    am_scene_node *node = am_new_userdata(L, am_scene_node);
    if (nargs > 0) {
        am_set_scene_node_child(L, node);
    }
    return 1;
}

static int create_hidden_node(lua_State *L) {
    int nargs = am_check_nargs(L, 0);
    am_scene_node *node = am_new_userdata(L, am_scene_node);
    node->recursion_limit = -1;
    if (nargs > 0) {
        am_set_scene_node_child(L, node);
    }
    return 1;
}

static int create_group_node(lua_State *L) {
    int nargs = am_check_nargs(L, 0);
    am_scene_node *node = am_new_userdata(L, am_scene_node);
    for (int i = 0; i < nargs; i++) {
        am_scene_node *child = am_get_userdata(L, am_scene_node, i+1);
        am_node_child child_slot;
        child_slot.child = child;
        child_slot.ref = node->ref(L, i+1); // ref from node to child
        node->children.push_back(L, child_slot);
    }
    return 1;
}

static int child_pair_next(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    int i = luaL_checkinteger(L, 2);
    if (i >= 0 && i < node->children.size) {
        lua_pushinteger(L, i+1);
        node->children.arr[i].child->push(L);
        return 2;
    } else {
        lua_pushnil(L);
        return 1;
    }
}

static int child_pairs(lua_State *L) {
    lua_pushcclosure(L, child_pair_next, 0);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);
    return 3;
}

static int get_child(lua_State *L) {
    am_check_nargs(L, 2);
    am_scene_node *node = am_get_userdata(L, am_scene_node, 1);
    int i = luaL_checkinteger(L, 2);
    if (i >= 1 && i <= node->children.size) {
        node->children.arr[i-1].child->push(L);
        return 1;
    } else {
        return 0;
    }
}

static void get_num_children(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    lua_pushinteger(L, node->children.size);
}

static am_property num_children_property = {get_num_children, NULL};

static void get_actions(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    if (node->actions_ref == LUA_NOREF) {
        lua_pushnil(L);
    } else {
        node->pushref(L, node->actions_ref);
    }
}

static void set_actions(lua_State *L, void *obj) {
    am_scene_node *node = (am_scene_node*)obj;
    if (node->actions_ref == LUA_NOREF) {
        node->actions_ref = node->ref(L, 3);
    } else {
        node->reref(L, node->actions_ref, 3);
    }
}

static am_property actions_property = {get_actions, set_actions};

static void register_scene_node_mt(lua_State *L) {
    lua_newtable(L);

    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    lua_pushcclosure(L, child_pairs, 0);
    lua_setfield(L, -2, "child_pairs");
    lua_pushcclosure(L, get_child, 0);
    lua_setfield(L, -2, "child");

    am_register_property(L, "num_children", &num_children_property);
    am_register_property(L, "_actions", &actions_property);

    lua_pushcclosure(L, append_child, 0);
    lua_setfield(L, -2, "append");
    lua_pushcclosure(L, prepend_child, 0);
    lua_setfield(L, -2, "prepend");
    lua_pushcclosure(L, remove_child, 0);
    lua_setfield(L, -2, "remove");
    lua_pushcclosure(L, replace_child, 0);
    lua_setfield(L, -2, "replace");
    lua_pushcclosure(L, remove_all_children, 0);
    lua_setfield(L, -2, "remove_all");
    
    lua_pushcclosure(L, am_create_bind_float_node, 0);
    lua_setfield(L, -2, "bind_float");
    lua_pushcclosure(L, am_create_bind_mat2_node, 0);
    lua_setfield(L, -2, "bind_mat2");
    lua_pushcclosure(L, am_create_bind_mat3_node, 0);
    lua_setfield(L, -2, "bind_mat3");
    lua_pushcclosure(L, am_create_bind_mat4_node, 0);
    lua_setfield(L, -2, "bind_mat4");
    lua_pushcclosure(L, am_create_bind_vec2_node, 0);
    lua_setfield(L, -2, "bind_vec2");
    lua_pushcclosure(L, am_create_bind_vec3_node, 0);
    lua_setfield(L, -2, "bind_vec3");
    lua_pushcclosure(L, am_create_bind_vec4_node, 0);
    lua_setfield(L, -2, "bind_vec4");
    lua_pushcclosure(L, am_create_bind_array_node, 0);
    lua_setfield(L, -2, "bind_array");
    lua_pushcclosure(L, am_create_bind_sampler2d_node, 0);
    lua_setfield(L, -2, "bind_sampler2d");
    lua_pushcclosure(L, am_create_program_node, 0);
    lua_setfield(L, -2, "bind_program");
    lua_pushcclosure(L, create_wrap_node, 0);
    lua_setfield(L, -2, "wrap");
    lua_pushcclosure(L, create_hidden_node, 0);
    lua_setfield(L, -2, "hidden");

    lua_pushcclosure(L, am_create_read_mat2_node, 0);
    lua_setfield(L, -2, "read_mat2");
    lua_pushcclosure(L, am_create_read_mat3_node, 0);
    lua_setfield(L, -2, "read_mat3");
    lua_pushcclosure(L, am_create_read_mat4_node, 0);
    lua_setfield(L, -2, "read_mat4");

    lua_pushcclosure(L, am_create_translate_node, 0);
    lua_setfield(L, -2, "translate");
    lua_pushcclosure(L, am_create_scale_node, 0);
    lua_setfield(L, -2, "scale");
    lua_pushcclosure(L, am_create_rotate_node, 0);
    lua_setfield(L, -2, "rotate");
    lua_pushcclosure(L, am_create_mult_mat4_node, 0);
    lua_setfield(L, -2, "mult_mat4");
    lua_pushcclosure(L, am_create_lookat_node, 0);
    lua_setfield(L, -2, "lookat");
    lua_pushcclosure(L, am_create_billboard_node, 0);
    lua_setfield(L, -2, "billboard");

    lua_pushcclosure(L, am_create_blend_node, 0);
    lua_setfield(L, -2, "blend");

    lua_pushcclosure(L, am_create_depth_pass_node, 0);
    lua_setfield(L, -2, "depth_pass");
    lua_pushcclosure(L, am_create_cull_face_node, 0);
    lua_setfield(L, -2, "cull_face");
    lua_pushcclosure(L, am_create_cull_sphere_node, 0);
    lua_setfield(L, -2, "cull_sphere");

    lua_pushcclosure(L, am_create_pass_filter_node, 0);
    lua_setfield(L, -2, "pass");

    am_register_metatable(L, "scene_node", MT_am_scene_node, 0);
}

void am_open_scene_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"group", create_group_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_scene_node_mt(L);
}
