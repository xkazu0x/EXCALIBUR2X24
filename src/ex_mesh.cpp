#include "ex_mesh.h"
#include "ex_logger.h"
#include "ex_utils.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>
#include <stdexcept>

namespace std {
    template <>
    struct hash<ex::vertex> {
        size_t operator()(ex::vertex const &vertex) const {
            size_t seed = 0;
            ex::utils::hash_combine(seed, vertex.position, vertex.color, vertex.uv, vertex.normal);
            return seed;
        }
    };
}

void
ex::mesh::load_file(const char *path) {
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, error;
    
    bool result = tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &error, path);
    if (!warn.empty()) { EXWARN("Mesh loader: %s", warn.c_str()); }
    if (!error.empty()) { EXERROR("Mesh loader: %s", error.c_str()); }
    if (!result) throw std::runtime_error(error);

    m_vertices.clear();
    m_indices.clear();

    std::unordered_map<ex::vertex , uint32_t> unique_vertices{};
    for (const tinyobj::shape_t &shape : shapes) {
        for (const tinyobj::index_t &index : shape.mesh.indices) {
            ex::vertex vertex = {};

            if (index.vertex_index >= 0) {
                vertex.position = {
                    attributes.vertices[3 * index.vertex_index + 0],
                    attributes.vertices[3 * index.vertex_index + 1],
                    attributes.vertices[3 * index.vertex_index + 2],
                };
                
                vertex.color = {
                    attributes.colors[3 * index.vertex_index + 0],
                    attributes.colors[3 * index.vertex_index + 1],
                    attributes.colors[3 * index.vertex_index + 2],
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.uv = {
                    attributes.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attributes.texcoords[2 * index.texcoord_index + 1],
                };
            }

            if (index.normal_index >= 0) {                
                vertex.normal = {
                    attributes.normals[3 * index.normal_index + 0],
                    attributes.normals[3 * index.normal_index + 1],
                    attributes.normals[3 * index.normal_index + 2],
                };
            }

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }
            m_indices.push_back(unique_vertices[vertex]);
        }
    }    
}

void
ex::mesh::load_array(std::vector<ex::vertex> &vertices,
                     std::vector<uint32_t> &indices) {
    m_vertices = vertices;
    m_indices = indices;
}
