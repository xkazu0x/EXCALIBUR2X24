#include "ex_camera.h"
#include <glm/gtx/vector_angle.hpp>

void
ex::camera::set_speed(float speed) {
    m_speed = speed;
}

void
ex::camera::set_sens(float sens) {
    m_sens = sens;
}

void
ex::camera::set_position(glm::vec3 position) {
    m_position = position;
}

void
ex::camera::set_target(glm::vec3 target) {
    m_target = target;
}

void
ex::camera::set_up(glm::vec3 up) {
    m_up = up;
}

void
ex::camera::set_perspective(float fov, float aspect, float znear, float zfar) {
    m_projection = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    m_fov = fov;
    m_znear = znear;
    m_zfar = zfar;
}

void
ex::camera::update_matrix(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    m_projection = glm::perspective(glm::radians(m_fov), aspect, m_znear, m_zfar);
    m_view = glm::lookAt(m_position, m_position - m_target, m_up);
}

void
ex::camera::update_input(ex::input *input, float delta) {
    if (input->key_down(EX_KEY_TAB)) m_locked = true;
    else m_locked = false;
    
    if (!m_locked) {
        if (input->key_down(EX_KEY_W)) {
            glm::vec3 left = glm::normalize(glm::cross(m_target, m_up));
            glm::vec3 forward = glm::normalize(glm::cross(left, m_up));        
            m_position += m_speed * delta * forward;
        }
    
        if (input->key_down(EX_KEY_S)) {
            glm::vec3 left = glm::normalize(glm::cross(m_target, m_up));
            glm::vec3 forward = glm::normalize(glm::cross(left, m_up));
            m_position += m_speed * delta * -forward;
        }
    
        if (input->key_down(EX_KEY_A)) m_position += m_speed * delta * glm::normalize(glm::cross(m_target, m_up));
        if (input->key_down(EX_KEY_D)) m_position += m_speed * delta * -glm::normalize(glm::cross(m_target, m_up));

        int mx, my;
        input->mouse_position(&mx, &my);

        glm::vec2 current_mouse_pos = glm::vec2((float)mx, (float)my);
        m_old_mouse_pos = glm::vec2((float)m_width / 2, (float)m_height / 2);
    
        glm::vec2 mouse_delta = current_mouse_pos - m_old_mouse_pos;
        //float xrot = m_sens * static_cast<float>((mouse_delta.x - (m_height / 2)) / m_height);

        glm::vec3 left = glm::normalize(glm::cross(m_target, m_up));
        m_target = glm::rotate(m_target, glm::radians(mouse_delta.y * m_sens * delta), -left);
        m_target = glm::rotate(m_target, glm::radians(mouse_delta.x * m_sens * delta), -m_up);
    }
}

void
ex::camera::rotate(glm::vec3 rotation) {
    m_target = glm::rotate(m_target, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    m_target = glm::rotate(m_target, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    m_target = glm::rotate(m_target, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
}
