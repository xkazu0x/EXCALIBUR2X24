#pragma once

#include "vk_backend.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace ex::vulkan {    
    class descriptor_pool {
    public:
        void add_size(VkDescriptorType type, uint32_t count);
        void create(ex::vulkan::backend *backend, uint32_t max_sets);
        void destroy(ex::vulkan::backend *backend);

        VkDescriptorPool handle() { return m_handle; }
            
    private:
        VkDescriptorPool m_handle;
        std::vector<VkDescriptorPoolSize> m_pool_sizes;
    };

    class descriptor_set_layout {
    public:
        void add_binding(uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage_flags);
        void create(ex::vulkan::backend *backend);
        void destroy(ex::vulkan::backend *backend);

        VkDescriptorSetLayout& handle() { return m_handle; }
        
    private:
        VkDescriptorSetLayout m_handle;
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
    };
    
    class descriptor_set {
    public:
        void allocate(ex::vulkan::backend *backend, ex::vulkan::descriptor_pool *pool, ex::vulkan::descriptor_set_layout *set_layout);
        void write_buffer(uint32_t binding, VkDescriptorType type, VkDescriptorBufferInfo *buffer_info);
        void write_image(uint32_t binding, VkDescriptorType type, VkDescriptorImageInfo *image_info);
        void update(ex::vulkan::backend *backend);

        VkDescriptorSet handle() { return m_handle; }
        
    private:
        VkDescriptorSet m_handle;
        std::vector<VkWriteDescriptorSet> m_writes;
    };
}
