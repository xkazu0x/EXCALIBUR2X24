#pragma once

#include "ex_vertex.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace ex::vulkan {
    class vbo {
    public:
        void load(std::vector<ex::vertex> vertices);
        void create(VkDevice logical_device,
                    VkPhysicalDevice physical_device,
                    VkAllocationCallbacks *allocator,
                    VkCommandPool command_pool,
                    VkQueue queue);
        void destroy(VkDevice logical_device,
                     VkAllocationCallbacks *allocator);

        VkBuffer handle();
        
    private:
        VkBuffer m_handle;
        VkDeviceMemory m_memory;
        VkDeviceSize m_size;
        std::vector<ex::vertex> m_data;
    };
}
