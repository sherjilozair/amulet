struct am_vec2 {
    glm::vec2 v;
    am_vec2() {}
    am_vec2(float *val) {
        v = glm::make_vec2(val);
    }
};

struct am_vec3 {
    glm::vec3 v;
    am_vec3() {}
    am_vec3(float *val) {
        v = glm::make_vec3(val);
    }
};

struct am_vec4 {
    glm::vec4 v;
    am_vec4() {}
    am_vec4(float *val) {
        v = glm::make_vec4(val);
    }
};

struct am_mat2 {
    glm::mat2 m;
    am_mat2() {}
    am_mat2(float *val) {
        m = glm::make_mat2(val);
    }
};

struct am_mat3 {
    glm::mat3 m;
    am_mat3() {}
    am_mat3(float *val) {
        m = glm::make_mat3(val);
    }
};

struct am_mat4 {
    glm::mat4 m;
    am_mat4() {}
    am_mat4(float *val) {
        m = glm::make_mat4(val);
    }
};

struct am_quat {
    glm::quat q;
    am_quat() {}
};

bool am_sphere_visible(glm::mat4 &matrix, glm::vec3 &center, float radius);

void am_open_math_module(lua_State *L);
