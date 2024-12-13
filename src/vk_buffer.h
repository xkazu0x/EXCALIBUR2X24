#pragma once

#include "vk_backend.h"
#include "ex_vertex.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace ex::vulkan {
    class buffer {
    public:
        void set_usage(VkBufferUsageFlags usage);
        void set_properties(VkMemoryPropertyFlags properties);
        void build(ex::vulkan::backend *backend, VkDeviceSize size);
        void bind(ex::vulkan::backend *backend, VkDeviceSize offset = 0);
        void map(ex::vulkan::backend *backend, VkDeviceSize offset = 0);
        void unmap(ex::vulkan::backend *backend);
        void copy_to(void *data, VkDeviceSize size);
        void copy_buffer(VkCommandBuffer command_buffer, VkBuffer buffer, VkDeviceSize size);
        void destroy(ex::vulkan::backend *backend);
        
        VkBuffer handle() { return m_handle; }
        VkDescriptorBufferInfo *get_descriptor_info();
        
    private:
        VkBuffer m_handle;
        VkDeviceMemory m_memory;
        VkDescriptorBufferInfo m_descriptor_info;
        VkBufferUsageFlags m_usage;
        VkMemoryPropertyFlags m_properties;
        VkDeviceSize m_size;
        void *m_mapped;
    };
}
