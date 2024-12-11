#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

namespace ex {
    class vertex {
    public:
        vertex() = default;
        vertex(glm::vec3 position, glm::vec3 color, glm::vec2 uv, glm::vec3 normal);
        ~vertex();

        bool operator==(const vertex &other) const;
        
        static std::vector<VkVertexInputBindingDescription> get_binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
        
    public:
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 uv;
        glm::vec3 normal;
    };
}
