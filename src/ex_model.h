#pragma once

#include "ex_mesh.h"
#include "vk_buffer.h"

namespace ex {
    class model {
    public:
        void load(const char *file);
        void create(VkDevice logical_device,
                    VkPhysicalDevice physical_device,
                    VkAllocationCallbacks *allocator,
                    VkCommandPool command_Pool,
                    VkQueue queue);
        void destroy(VkDevice logical_device,
                     VkAllocationCallbacks *allocator);
        void bind(VkCommandBuffer command_bufer);
        void draw(VkCommandBuffer command_buffer);

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
