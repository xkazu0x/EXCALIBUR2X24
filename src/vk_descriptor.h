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

        VkDescriptorPool& handle() { return m_handle; }
            
    private:
        VkDescriptorPool m_handle;
        std::vector<VkDescriptorPoolSize> m_pool_sizes;
    };

    class descriptor_set {
    public:
        void add_binding(uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage_flags, const VkSampler *immutable_samplers);
        void build_layout(ex::vulkan::backend *backend);
        void destroy_layout(ex::vulkan::backend *backend);
        
        void allocate(ex::vulkan::backend *backend, ex::vulkan::descriptor_pool *pool);
        void add_write_set(uint32_t binding, VkDescriptorType type, const VkDescriptorImageInfo *image_info, const VkDescriptorBufferInfo *buffer_info);
        void update(ex::vulkan::backend *backend);

        VkDescriptorSet handle() { return m_handle; }
        VkDescriptorSetLayout layout() { return m_layout; }
        
    private:
        VkDescriptorSet m_handle;
        VkDescriptorSetLayout m_layout;
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
        std::vector<VkWriteDescriptorSet> m_write_sets;
    };
}
