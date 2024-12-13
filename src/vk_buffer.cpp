#include "vk_buffer.h"
#include "vk_common.h"
#include <memory>

void
ex::vulkan::buffer::set_usage(VkBufferUsageFlags usage) {
    m_usage = usage;
}

void
ex::vulkan::buffer::set_properties(VkMemoryPropertyFlags properties) {
    m_properties = properties;
}

void
ex::vulkan::buffer::build(ex::vulkan::backend *backend, VkDeviceSize size) {
    m_size = size;
    
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = m_size;
    buffer_create_info.usage = m_usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;
    VK_CHECK(vkCreateBuffer(backend->logical_device(),
                            &buffer_create_info,
                            backend->allocator(),
                            &m_handle));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(backend->logical_device(),
                                  m_handle,
                                  &memory_requirements);

    uint32_t memory_type_index = backend->get_memory_type_index(memory_requirements, m_properties);
    
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    VK_CHECK(vkAllocateMemory(backend->logical_device(),
                              &memory_allocate_info,
                              backend->allocator(),
                              &m_memory));
}

void
ex::vulkan::buffer::bind(ex::vulkan::backend *backend, VkDeviceSize offset) {
    vkBindBufferMemory(backend->logical_device(),
                       m_handle,
                       m_memory,
                       offset);
}

void
ex::vulkan::buffer::map(ex::vulkan::backend *backend, VkDeviceSize offset) {
    vkMapMemory(backend->logical_device(),
                m_memory,
                offset,
                m_size,
                0,
                &m_mapped);
}

void
ex::vulkan::buffer::unmap(ex::vulkan::backend *backend) {
    vkUnmapMemory(backend->logical_device(), m_memory);
    //m_mapped = nullptr;
}

void
ex::vulkan::buffer::copy_to(void *data, VkDeviceSize size) {
    memcpy(m_mapped, data, (size_t) size);
}

void
ex::vulkan::buffer::copy_buffer(VkCommandBuffer command_buffer,
                                VkBuffer buffer,
                                VkDeviceSize size) {
    VkBufferCopy buffer_copy = {};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = size;
    vkCmdCopyBuffer(command_buffer, buffer, m_handle, 1, &buffer_copy);
}

void
ex::vulkan::buffer::destroy(ex::vulkan::backend *backend) {
    if (m_handle) {
        vkDestroyBuffer(backend->logical_device(),
                        m_handle,
                        backend->allocator());
    }
    if (m_memory) {
        vkFreeMemory(backend->logical_device(),
                     m_memory,
                     backend->allocator());
    }
}

VkDescriptorBufferInfo*
ex::vulkan::buffer::get_descriptor_info() {
    m_descriptor_info = {};
    m_descriptor_info.buffer = m_handle;
    m_descriptor_info.offset = 0;
    m_descriptor_info.range = m_size;

    return &m_descriptor_info;
}
