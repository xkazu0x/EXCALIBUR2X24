#pragma once

#include "ex_component.hpp"
#include "ex_input.h"

namespace ex {
    class camera {
    public:
        void set_position(glm::vec3 position);
        void set_target(glm::vec3 target);
        void set_up(glm::vec3 up);

        void set_speed(float speed);
        void set_sens(float sens);
        
        void set_perspective(float fov, float aspect, float znear, float zfar);
        void update_aspect_ratio(float aspect);

        void update(ex::input *input, float delta);
        
        glm::mat4 get_view() { return m_view; }
        glm::mat4 get_projection() { return m_projection; }

    private:
        void update_matrix();
        
    private:
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
    };
}
