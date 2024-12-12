#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace ex::comp {
    struct transform {
        glm::vec3 translation;
        glm::vec3 rotation;
        glm::vec3 scale;
        
        glm::mat4 matrix() {
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, translation);
            transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            transform = glm::scale(transform, scale);
            return transform;
        }
    };
}
