#pragma once

#include "ex_mesh.h"
#include "vk_buffer.h"

namespace ex::vulkan {
    class model {
    public:
        void load(const char *file);
        void load_array(std::vector<ex::vertex> &vertices,
                        std::vector<uint32_t> &indices);
        void create(VkDevice logical_device,
                    VkPhysicalDevice physical_device,
                    VkAllocationCallbacks *allocator,
                    VkCommandPool command_Pool,
                    VkQueue queue);
        void destroy(VkDevice logical_device,
                     VkAllocationCallbacks *allocator);
        void bind(VkCommandBuffer command_bufer);
        void draw(VkCommandBuffer command_buffer);

        uint32_t vertex_count() { return m_vertex_count; }
        uint32_t index_count() { return m_index_count; }
        
    private:
        void create_vertex_buffer(VkDevice logical_device,
                                  VkPhysicalDevice physical_device,
                                  VkAllocationCallbacks *allocator,
                                  VkCommandPool command_pool,
                                  VkQueue queue);
        void create_index_buffer(VkDevice logical_device,
                                 VkPhysicalDevice physical_device,
                                 VkAllocationCallbacks *allocator,
                                 VkCommandPool command_pool,
                                 VkQueue queue);
        
    private:
        ex::mesh m_mesh;
        ex::vulkan::buffer m_vertex_buffer;
        ex::vulkan::buffer m_index_buffer;
        uint32_t m_vertex_count;
        uint32_t m_index_count;
    };
}
