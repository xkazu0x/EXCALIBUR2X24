#pragma once

#include "ex_mesh.h"
#include "vk_backend.h"
#include "vk_buffer.h"

namespace ex::vulkan {
    class model {
    public:
        void create(ex::vulkan::backend *backend, ex::mesh *mesh);
        void destroy(ex::vulkan::backend *backend);
        void bind(VkCommandBuffer command_bufer);
        void draw(VkCommandBuffer command_buffer);

        uint32_t vertex_count() { return m_vertex_count; }
        uint32_t index_count() { return m_index_count; }
        
    private:
        void create_vertex_buffer(ex::vulkan::backend *backend, std::vector<ex::vertex> vertices);
        void create_index_buffer(ex::vulkan::backend *backend, std::vector<uint32_t> indices);
        
    private:
        ex::vulkan::buffer m_vertex_buffer;
        ex::vulkan::buffer m_index_buffer;
        uint32_t m_vertex_count;
        uint32_t m_index_count;
    };
}
