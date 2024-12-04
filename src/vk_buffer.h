#pragma once

#include "ex_vertex.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace ex::vulkan {
    class buffer {
    public:
        void create(VkDevice logical_device,
                    VkPhysicalDevice physical_device,
                    VkAllocationCallbacks *allocator,
                    VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties);
        void copy_data(VkDevice logical_device,
                       const void *source,
                       VkDeviceSize size);
        void copy_buffer(VkDevice logical_device,
                         VkCommandPool command_pool,
                         VkQueue queue,
                         VkBuffer source_buffer,
                         VkDeviceSize source_buffer_size);        
        void destroy(VkDevice logical_device,
                     VkAllocationCallbacks *allocator);

        VkBuffer handle();
        
    private:
        VkBuffer m_handle;
        VkDeviceMemory m_memory;
    };
}
