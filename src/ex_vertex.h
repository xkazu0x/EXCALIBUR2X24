#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>

namespace ex {
    class vertex {
    public:
        vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uv, glm::vec3 normal)
            : pos(pos), color(color), uv(uv), normal(normal) {
        }

        static VkVertexInputBindingDescription get_binding_description() {
            VkVertexInputBindingDescription vertex_input_binding_description = {};
            vertex_input_binding_description.binding = 0;
            vertex_input_binding_description.stride = sizeof(ex::vertex);
            vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            
            return vertex_input_binding_description;
        }

        static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions() {
            std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions(4);
            vertex_input_attribute_descriptions[0].location = 0;
            vertex_input_attribute_descriptions[0].binding = 0;
            vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attribute_descriptions[0].offset = offsetof(ex::vertex, pos);
    
            vertex_input_attribute_descriptions[1].location = 1;
            vertex_input_attribute_descriptions[1].binding = 0;
            vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attribute_descriptions[1].offset = offsetof(ex::vertex, color);
            
            vertex_input_attribute_descriptions[2].location = 2;
            vertex_input_attribute_descriptions[2].binding = 0;
            vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            vertex_input_attribute_descriptions[2].offset = offsetof(ex::vertex, uv);

            vertex_input_attribute_descriptions[3].location = 3;
            vertex_input_attribute_descriptions[3].binding = 0;
            vertex_input_attribute_descriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            vertex_input_attribute_descriptions[3].offset = offsetof(ex::vertex, normal);
            
            return vertex_input_attribute_descriptions;
        }
        
    public:
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 uv;
        glm::vec3 normal;
    };
}
