#include "vk_image.h"
#include "vk_common.h"

void
ex::vulkan::image::set_type(VkImageType type) {
    m_type = type;
}

void
ex::vulkan::image::set_format(VkFormat format) {
    m_format = format;
}

void
ex::vulkan::image::set_extent(VkExtent2D extent) {
    m_extent = extent;
}

void
ex::vulkan::image::set_tiling(VkImageTiling tiling) {
    m_tiling = tiling;
}

void
ex::vulkan::image::set_usage(VkImageUsageFlags usage) {
    m_usage = usage;
}

void
ex::vulkan::image::set_layout(VkImageLayout layout) {
    m_layout = layout;
}

void
ex::vulkan::image::create(ex::vulkan::backend *backend) {
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.flags = 0;
    image_create_info.imageType = m_type;
    image_create_info.format = m_format;
    image_create_info.extent.width = m_extent.width;
    image_create_info.extent.height = m_extent.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = m_tiling;
    image_create_info.usage = m_usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.initialLayout = m_layout;
    VK_CHECK(vkCreateImage(backend->logical_device(),
                           &image_create_info,
                           backend->allocator(),
                           &m_handle));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(backend->logical_device(),
                                 m_handle,
                                 &memory_requirements);

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    uint32_t memory_type_index = backend->get_memory_type_index(memory_requirements, properties);
    
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = nullptr;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    VK_CHECK(vkAllocateMemory(backend->logical_device(),
                              &memory_allocate_info,
                              backend->allocator(),
                              &m_memory));
}

void
ex::vulkan::image::destroy(ex::vulkan::backend *backend) {
    if (m_view) {
        vkDestroyImageView(backend->logical_device(),
                           m_view,
                           backend->allocator());
    }
    
    if (m_handle) {
        vkDestroyImage(backend->logical_device(),
                       m_handle,
                       backend->allocator());
    }
    
    if (m_memory) {
        vkFreeMemory(backend->logical_device(),
                     m_memory,
                     backend->allocator());
    }
}

void
ex::vulkan::image::bind(ex::vulkan::backend *backend) {
    vkBindImageMemory(backend->logical_device(), m_handle, m_memory, 0);
}

void
ex::vulkan::image::change_layout(VkCommandBuffer command_buffer,
                                 VkImageLayout layout,
                                 VkImageAspectFlags aspect_mask) {
    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    //image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    //image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = 0;
    image_memory_barrier.oldLayout = m_layout;
    image_memory_barrier.newLayout = layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = m_handle;
    image_memory_barrier.subresourceRange.aspectMask = aspect_mask;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &image_memory_barrier);
    
    m_layout = layout;
}

void
ex::vulkan::image::copy_buffer_to(VkCommandBuffer command_buffer,
                                  VkBuffer buffer,
                                  VkImageAspectFlags aspect_mask,
                                  VkExtent2D extent) {
    VkBufferImageCopy buffer_image_copy = {};
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = aspect_mask;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset = { 0, 0, 0 };
    buffer_image_copy.imageExtent.width = extent.width;
    buffer_image_copy.imageExtent.height = extent.height;
    buffer_image_copy.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(command_buffer,
                           buffer,
                           m_handle,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // m_layout
                           1,
                           &buffer_image_copy);
}

void
ex::vulkan::image::create_view(ex::vulkan::backend *backend,
                               VkImageViewType view_type,
                               VkImageAspectFlags aspect_flags) {
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = m_handle;
    image_view_create_info.viewType = view_type;
    image_view_create_info.format = m_format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = aspect_flags;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    VK_CHECK(vkCreateImageView(backend->logical_device(),
                               &image_view_create_info,
                               backend->allocator(),
                               &m_view));
}
