#include "ex_vertex.h"
        
ex::vertex::vertex(glm::vec3 position, glm::vec3 color, glm::vec2 uv, glm::vec3 normal)
    : position(position), color(color), uv(uv), normal(normal) {
}

ex::vertex::~vertex() {
}

bool
ex::vertex::operator==(const vertex &other) const {
    return (position == other.position &&
            color == other.color &&
            uv == other.uv &&
            normal == other.normal);
}
        
std::vector<VkVertexInputBindingDescription>
ex::vertex::get_binding_descriptions() {
    std::vector<VkVertexInputBindingDescription> binding_descriptions;
    binding_descriptions.push_back({0, sizeof(ex::vertex), VK_VERTEX_INPUT_RATE_VERTEX});
                
    return binding_descriptions;
}

std::vector<VkVertexInputAttributeDescription>
ex::vertex::get_attribute_descriptions() {
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    attribute_descriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ex::vertex, position)});
    attribute_descriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ex::vertex, color)});
    attribute_descriptions.push_back({2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ex::vertex, uv)});
    attribute_descriptions.push_back({3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(ex::vertex, normal)});
            
    return attribute_descriptions;
}
