#include "vk_texture.h"
#include "vk_common.h"
#include "ex_logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stdexcept>

void
ex::vulkan::texture::create(ex::vulkan::backend *backend, const char *file_path) {
    int width, height, channels;
    stbi_uc *texture_data = stbi_load(file_path, &width, &height, &channels, STBI_rgb_alpha);
    if (!texture_data) {
        EXFATAL("Failed to load texture image");
        throw std::runtime_error("Failed to load texture image");
    }

    uint32_t texture_width = static_cast<uint32_t>(width);
    uint32_t texture_height = static_cast<uint32_t>(height);
    VkDeviceSize texture_size = sizeof(uint32_t) * texture_width * texture_height;

    ex::vulkan::buffer staging_buffer;
    staging_buffer.set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    staging_buffer.set_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging_buffer.build(backend, texture_size);
    
    staging_buffer.bind(backend);
    staging_buffer.map(backend);
    staging_buffer.copy_to(texture_data, texture_size);
    staging_buffer.unmap(backend);
    stbi_image_free(texture_data);

    m_image.set_type(VK_IMAGE_TYPE_2D);
    m_image.set_format(VK_FORMAT_R8G8B8A8_UNORM);
    m_image.set_extent({texture_width, texture_height});
    m_image.set_tiling(VK_IMAGE_TILING_OPTIMAL);
    m_image.set_usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    m_image.set_layout(VK_IMAGE_LAYOUT_PREINITIALIZED);
    m_image.create(backend);

    m_image.bind(backend);
    VkCommandBuffer cmd_layout_transfer = backend->begin_single_time_commands();
    m_image.change_layout(cmd_layout_transfer,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);
    backend->end_single_time_commands(cmd_layout_transfer);

    VkCommandBuffer cmd_copy_buffer = backend->begin_single_time_commands();
    m_image.copy_buffer_to(cmd_copy_buffer,
                           staging_buffer.handle(),
                           VK_IMAGE_ASPECT_COLOR_BIT,
                           {texture_width, texture_height});
    backend->end_single_time_commands(cmd_copy_buffer);

    VkCommandBuffer cmd_layout_shader = backend->begin_single_time_commands();
    m_image.change_layout(cmd_layout_shader,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_IMAGE_ASPECT_COLOR_BIT);
    backend->end_single_time_commands(cmd_layout_shader);
    staging_buffer.destroy(backend);

    m_image.create_view(backend, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
    
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    VK_CHECK(vkCreateSampler(backend->logical_device(),
                             &sampler_create_info,
                             backend->allocator(),
                             &m_sampler));
}

void
ex::vulkan::texture::destroy(ex::vulkan::backend *backend) {
    vkDeviceWaitIdle(backend->logical_device());
    if (m_sampler) {
        vkDestroySampler(backend->logical_device(),
                         m_sampler,
                         backend->allocator());
    }
    
    m_image.destroy(backend);
}

VkDescriptorImageInfo *
ex::vulkan::texture::get_descriptor_info() {
    m_descriptor_info = {};
    m_descriptor_info.sampler = m_sampler;
    m_descriptor_info.imageView = m_image.view();
    m_descriptor_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    return &m_descriptor_info;
}
