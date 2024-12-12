#include "vk_descriptor.h"
#include "vk_common.h"

void
ex::vulkan::descriptor_pool::add_size(VkDescriptorType type,
                                      uint32_t count) {
    VkDescriptorPoolSize descriptor_pool_size = {};
    descriptor_pool_size.type = type;
    descriptor_pool_size.descriptorCount = count;
    m_pool_sizes.push_back(descriptor_pool_size);
}

void
ex::vulkan::descriptor_pool::create(ex::vulkan::backend *backend) {
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(m_pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes = m_pool_sizes.data();
    VK_CHECK(vkCreateDescriptorPool(backend->logical_device(),
                                    &descriptor_pool_create_info,
                                    backend->allocator(),
                                    &m_handle));
}

void
ex::vulkan::descriptor_pool::destroy(ex::vulkan::backend *backend) {
    vkDeviceWaitIdle(backend->logical_device());
    if (m_handle) vkDestroyDescriptorPool(backend->logical_device(),
                                          m_handle,
                                          backend->allocator());
}

void
ex::vulkan::descriptor_set_layout::add_binding(uint32_t binding,
                                               VkDescriptorType type,
                                               uint32_t count,
                                               VkShaderStageFlags stage_flags,
                                               const VkSampler *immutable_samplers) {
    VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
    descriptor_set_layout_binding.binding = binding;
    descriptor_set_layout_binding.descriptorType = type;
    descriptor_set_layout_binding.descriptorCount = count;
    descriptor_set_layout_binding.stageFlags = stage_flags;
    descriptor_set_layout_binding.pImmutableSamplers = immutable_samplers;
    m_bindings.push_back(descriptor_set_layout_binding);
}

void
ex::vulkan::descriptor_set_layout::create(ex::vulkan::backend *backend) {
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = nullptr;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(m_bindings.size());
    descriptor_set_layout_create_info.pBindings = m_bindings.data();
    VK_CHECK(vkCreateDescriptorSetLayout(backend->logical_device(),
                                         &descriptor_set_layout_create_info,
                                         backend->allocator(),
                                         &m_handle));
}

void
ex::vulkan::descriptor_set_layout::destroy(ex::vulkan::backend *backend) {
    vkDeviceWaitIdle(backend->logical_device());
    if (m_handle) vkDestroyDescriptorSetLayout(backend->logical_device(),
                                               m_handle,
                                               backend->allocator());
}

void
ex::vulkan::descriptor_set::allocate(ex::vulkan::backend *backend,
                                     ex::vulkan::descriptor_pool *pool,
                                     ex::vulkan::descriptor_set_layout *set_layout) {    
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = pool->handle();
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = set_layout->handle();
    VK_CHECK(vkAllocateDescriptorSets(backend->logical_device(),
                                      &descriptor_set_allocate_info,
                                      &m_handle));
}

void
ex::vulkan::descriptor_set::add_write_set(uint32_t binding,
                                          VkDescriptorType type,
                                          const VkDescriptorImageInfo *image_info,
                                          const VkDescriptorBufferInfo *buffer_info) {
    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = nullptr;
    write_descriptor_set.dstSet = m_handle;
    write_descriptor_set.dstBinding = binding;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.descriptorType = type;
    write_descriptor_set.pImageInfo = image_info;
    write_descriptor_set.pBufferInfo = buffer_info;
    write_descriptor_set.pTexelBufferView = nullptr;
    m_write_sets.push_back(write_descriptor_set);
}

void
ex::vulkan::descriptor_set::update(ex::vulkan::backend *backend) {
    vkUpdateDescriptorSets(backend->logical_device(),
                           m_write_sets.size(),
                           m_write_sets.data(),
                           0, nullptr);
}
