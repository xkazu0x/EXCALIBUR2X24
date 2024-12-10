#pragma once

#include "ex_component.hpp"

namespace ex {
    class camera {
    public:
        void set_translation(glm::vec3 translation);
        void set_rotation(glm::vec3 rotation);
        void set_perspective(float fov, float aspect, float znear, float zfar);
        
        void update_aspect_ratio(float aspect);

        glm::mat4 get_view();
        glm::mat4 get_projection();

    private:
        void update_matrix_view();
        
    private:
        ex::comp::transform m_transform;
        glm::mat4 m_view;
        glm::mat4 m_projection;
        float m_fov;
        float m_znear;
        float m_zfar;
    };
}
