#include "vk_texture.h"

VkDescriptorImageInfo *
ex::vulkan::texture::get_descriptor_info() {
    m_descriptor_info = {};
    m_descriptor_info.sampler = sampler;
    m_descriptor_info.imageView = image.view();
    m_descriptor_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    return &m_descriptor_info;
}
