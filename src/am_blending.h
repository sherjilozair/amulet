struct am_blend_node : am_scene_node {
    am_blend_mode mode;
    virtual void render(am_render_state *rstate);
};

int am_create_blend_node(lua_State *L);

void am_open_blending_module(lua_State *L);
