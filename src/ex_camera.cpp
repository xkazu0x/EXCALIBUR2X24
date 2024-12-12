#include "ex_camera.h"

void
ex::camera::set_translation(glm::vec3 translation) {
    m_transform.translation = translation;
    update_matrix_view();
}

void
ex::camera::set_rotation(glm::vec3 rotation) {
    m_transform.rotation = rotation;
    update_matrix_view();
}
        
void
ex::camera::set_perspective(float fov, float aspect, float znear, float zfar) {
    m_fov = fov;
    m_znear = znear;
    m_zfar = zfar;
    m_projection = glm::perspective(glm::radians(fov), aspect, znear, zfar);
}

void
ex::camera::translate(glm::vec3 translation) {
    m_transform.translation += translation;
    update_matrix_view();
}

void
ex::camera::update_aspect_ratio(float aspect) {
    m_projection = glm::perspective(glm::radians(m_fov), aspect, m_znear, m_zfar);
}

glm::mat4
ex::camera::get_view() { return m_view; }

glm::mat4
ex::camera::get_projection() { return m_projection; }

void
ex::camera::update_matrix_view() {
    m_view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                         glm::vec3(0.0f, 0.0f, 1.0f),
                         glm::vec3(0.0f, -1.0f, 0.0f));
    m_view = glm::rotate(m_view, glm::radians(m_transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    m_view = glm::rotate(m_view, glm::radians(m_transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    m_view = glm::rotate(m_view, glm::radians(m_transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    m_view = glm::translate(m_view, m_transform.translation);
}
