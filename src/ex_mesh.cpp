#include "ex_mesh.h"
#include "ex_logger.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#include <stdexcept>

void
ex::mesh::create(const char *path) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn_message;
    std::string error_message;
    
    bool result = tinyobj::LoadObj(&attributes,
                                  &shapes,
                                  &materials,
                                  &warn_message,
                                  &error_message,
                                  path);
    if (!warn_message.empty()) EXWARN("Mesh loader: %s", warn_message);
    if (!error_message.empty()) EXERROR("Mesh loader: %s", error_message);
    if (!result) throw std::runtime_error(error_message);

    for (tinyobj::shape_t shape : shapes) {
        for (tinyobj::index_t index : shape.mesh.indices) {
            glm::vec3 pos = {
                attributes.vertices[3 * index.vertex_index + 0],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2],
            };

            m_vertices.push_back(ex::vertex(pos, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}));
            m_indices.push_back(m_indices.size());
        }
    }    
}

std::vector<ex::vertex>
ex::mesh::vertices() {
    return m_vertices;
}

std::vector<uint32_t>
ex::mesh::indices() {
    return m_indices;
}
