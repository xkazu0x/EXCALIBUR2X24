#include "ex_camera.h"
#include <glm/gtx/vector_angle.hpp>

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
ex::camera::set_speed(float speed) {
    m_speed = speed;
}

void
ex::camera::set_sens(float sens) {
    m_sens = sens;
}

void
ex::camera::set_perspective(float fov, float aspect, float znear, float zfar) {
    m_fov = fov;
    m_znear = znear;
    m_zfar = zfar;
    m_projection = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}

void
ex::camera::update_aspect_ratio(float aspect) {
    m_projection = glm::perspective(glm::radians(m_fov), aspect, m_znear, m_zfar);
}

void
ex::camera::update(ex::input *input, float delta) {
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

    if (input->key_down(EX_KEY_I)) {
        glm::vec3 new_target = glm::rotate(m_target, glm::radians(m_sens * delta), glm::normalize(glm::cross(m_target, m_up)));
        if (!(glm::angle(new_target, m_up) <= glm::radians(5.0f) || glm::angle(new_target, -m_up) <= glm::radians(5.0f))) {
            m_target = new_target;
        }
    }
    
    if (input->key_down(EX_KEY_K)) {
        glm::vec3 new_target = glm::rotate(m_target, glm::radians(-m_sens * delta), glm::normalize(glm::cross(m_target, m_up)));
        if (!(glm::angle(new_target, m_up) <= glm::radians(5.0f) || glm::angle(new_target, -m_up) <= glm::radians(5.0f))) {
            m_target = new_target;
        }
    }

    if (input->key_down(EX_KEY_J)) m_target = glm::rotate(m_target, glm::radians(m_sens * delta), m_up);
    if (input->key_down(EX_KEY_L)) m_target = glm::rotate(m_target, glm::radians(-m_sens * delta), m_up);

    m_view = glm::lookAt(m_position, m_position - m_target, m_up);
}
