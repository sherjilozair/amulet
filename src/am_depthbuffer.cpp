#include "amulet.h"

void am_depth_test_node::render(am_render_state *rstate) {
    am_depth_test_state old_state = rstate->active_depth_test_state;
    rstate->active_depth_test_state.set(func != AM_DEPTH_FUNC_ALWAYS, mask_enabled, func);
    render_children(rstate);
    rstate->active_depth_test_state.restore(&old_state);
}

static int create_depth_test_node(lua_State *L) {
    int nargs = am_check_nargs(L, 1);
    am_depth_test_node *node = am_new_userdata(L, am_depth_test_node);
    node->func = am_get_enum(L, am_depth_func, 1);
    node->mask_enabled = true;
    if (nargs > 1) {
        node->mask_enabled = lua_toboolean(L, 2);
    }
    return 1;
}

static void register_depth_test_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_metatable(L, "depth_test", MT_am_depth_test_node, MT_am_scene_node);
}

void am_open_depthbuffer_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"depth_test", create_depth_test_node},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);

    am_enum_value depth_func_enum[] = {
        {"never", AM_DEPTH_FUNC_NEVER},
        {"always", AM_DEPTH_FUNC_ALWAYS},
        {"equal", AM_DEPTH_FUNC_EQUAL},
        {"notequal", AM_DEPTH_FUNC_NOTEQUAL},
        {"less", AM_DEPTH_FUNC_LESS},
        {"lequal", AM_DEPTH_FUNC_LEQUAL},
        {"greater", AM_DEPTH_FUNC_GREATER},
        {"gequal", AM_DEPTH_FUNC_GEQUAL},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_depth_func, depth_func_enum);

    register_depth_test_node_mt(L);
}
