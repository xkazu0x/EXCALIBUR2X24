#include "vk_buffer.h"
#include "vk_common.h"

void
ex::vulkan::vbo::load(std::vector<ex::vertex> vertices) {
    m_size = sizeof(ex::vertex) * static_cast<uint32_t>(vertices.size());
    m_data = vertices;
}

void
ex::vulkan::vbo::create(VkDevice logical_device,
                        VkPhysicalDevice physical_device,
                        VkAllocationCallbacks *allocator,
                        VkCommandPool command_pool,
                        VkQueue queue) {
    // CREATE BUFFER
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = m_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
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
            (physical_device_memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
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

    // CREATE STAGING BUFFER
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    
    VkBufferCreateInfo staging_buffer_create_info = {};
    staging_buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buffer_create_info.size = m_size;
    staging_buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    staging_buffer_create_info.queueFamilyIndexCount = 0;
    staging_buffer_create_info.pQueueFamilyIndices = nullptr;
    VK_CHECK(vkCreateBuffer(logical_device,
                            &staging_buffer_create_info,
                            allocator,
                            &staging_buffer));

    // ALLOCATE STAGING MEMORY
    VkMemoryRequirements staging_memory_requirements;
    vkGetBufferMemoryRequirements(logical_device,
                                  staging_buffer,
                                  &staging_memory_requirements);
        
    uint32_t staging_memory_type_index = 0;
    // TODO: check error
    for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
        if ((staging_memory_requirements.memoryTypeBits & (1 << i)) &&
            (physical_device_memory_properties.memoryTypes[i].propertyFlags &
             (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            staging_memory_type_index = i;
            break;
        }
    }
            
    VkMemoryAllocateInfo staging_memory_allocate_info = {};
    staging_memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    staging_memory_allocate_info.allocationSize = staging_memory_requirements.size;
    staging_memory_allocate_info.memoryTypeIndex = staging_memory_type_index;
    VK_CHECK(vkAllocateMemory(logical_device,
                              &staging_memory_allocate_info,
                              allocator,
                              &staging_buffer_memory));

    // COPY DATA TO BUFFER
    vkBindBufferMemory(logical_device,
                       staging_buffer,
                       staging_buffer_memory,
                       0);

    void *data;
    vkMapMemory(logical_device,
                staging_buffer_memory,
                0,
                m_size,
                0,
                &data);
    memcpy(data, m_data.data(), static_cast<size_t>(m_size));
    vkUnmapMemory(logical_device, staging_buffer_memory);

    vkBindBufferMemory(logical_device,
                       m_handle,
                       m_memory,
                       0);

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
    buffer_copy.size = m_size;
    vkCmdCopyBuffer(command_buffer, staging_buffer, m_handle, 1, &buffer_copy);
    
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(logical_device, command_pool, 1, &command_buffer);    
    vkDestroyBuffer(logical_device, staging_buffer, allocator);
    vkFreeMemory(logical_device, staging_buffer_memory, allocator);
}

void
ex::vulkan::vbo::destroy(VkDevice logical_device,
                         VkAllocationCallbacks *allocator) {
    if (m_memory) {        
        vkFreeMemory(logical_device,
                     m_memory,
                     allocator);
    }
    
    if (m_handle) {
        vkDestroyBuffer(logical_device,
                        m_handle,
                        allocator);
    }
}

VkBuffer
ex::vulkan::vbo::handle() {
    return m_handle;
}
