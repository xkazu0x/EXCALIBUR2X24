#include "vk_buffer.h"
#include "vk_common.h"
#include <memory>

void
ex::vulkan::buffer::create(VkDevice logical_device,
                           VkPhysicalDevice physical_device,
                           VkAllocationCallbacks *allocator,
                           VkDeviceSize size,
                           VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties) {
    m_size = size;
    
    // CREATE BUFFER
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;
    VK_CHECK(vkCreateBuffer(logical_device,
                            &buffer_create_info,
                            allocator,
                            &m_handle));

    // ALLOCATE MEMORY
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(logical_device,
                                  m_handle,
                                  &memory_requirements);
    
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);
    
    uint32_t memory_type_index = 0;
    // TODO: check error
    for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
        if ((memory_requirements.memoryTypeBits & (1 << i)) &&
            (physical_device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            memory_type_index = i;
            break;
        }
    }
    
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    VK_CHECK(vkAllocateMemory(logical_device,
                              &memory_allocate_info,
                              allocator,
                              &m_memory));

    vkBindBufferMemory(logical_device,
                       m_handle,
                       m_memory,
                       0);
}

void
ex::vulkan::buffer::copy_data(VkDevice logical_device,
                              const void *source,
                              VkDeviceSize size) {
    void *data;
    vkMapMemory(logical_device,
                m_memory,
                0,
                size,
                0,
                &data);
    memcpy(data, source, (size_t) size);
    vkUnmapMemory(logical_device, m_memory);
}

void
ex::vulkan::buffer::copy_buffer(VkDevice logical_device,
                                VkCommandPool command_pool,
                                VkQueue queue,
                                VkBuffer source_buffer,
                                VkDeviceSize source_buffer_size) {
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    VK_CHECK(vkAllocateCommandBuffers(logical_device,
                                      &command_buffer_allocate_info,
                                      &command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

    VkBufferCopy buffer_copy;
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = source_buffer_size;
    vkCmdCopyBuffer(command_buffer, source_buffer, m_handle, 1, &buffer_copy);
    
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buffer);
}

void
ex::vulkan::buffer::destroy(VkDevice logical_device,
                            VkAllocationCallbacks *allocator) {
    if (m_handle) vkDestroyBuffer(logical_device, m_handle, allocator);
    if (m_memory) vkFreeMemory(logical_device, m_memory, allocator);
}

VkBuffer
ex::vulkan::buffer::handle() {
    return m_handle;
}

VkDescriptorBufferInfo*
ex::vulkan::buffer::get_descriptor_info() {
    m_descriptor_info = {};
    m_descriptor_info.buffer = m_handle;
    m_descriptor_info.offset = 0;
    m_descriptor_info.range = m_size;

    return &m_descriptor_info;
}
