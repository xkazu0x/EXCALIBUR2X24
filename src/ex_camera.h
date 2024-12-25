#pragma once

#include "ex_input.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace ex {
    class camera {
    public:
        void set_speed(float speed);
        void set_sens(float sens);

        void set_position(glm::vec3 position);
        void set_target(glm::vec3 target);
        void set_up(glm::vec3 up);
        void set_perspective(float fov, float aspect, float znear, float zfar);

        void update_matrix(uint32_t width, uint32_t height);
        void update_input(ex::input *input, float delta);
        
        void rotate(glm::vec3 rotation);
        bool is_locked() { return m_locked; }
        
        glm::mat4 get_view() { return m_view; }
        glm::mat4 get_projection() { return m_projection; }
        
    public:
        glm::vec3 m_position;
        glm::vec3 m_target;
        glm::vec3 m_up;

        float m_speed;
        float m_sens;
        
        glm::mat4 m_view;
        glm::mat4 m_projection;
        float m_fov;
        float m_znear;
        float m_zfar;

        uint32_t m_width;
        uint32_t m_height;
        glm::vec2 m_old_mouse_pos;

        bool m_locked {true};
    };
}
