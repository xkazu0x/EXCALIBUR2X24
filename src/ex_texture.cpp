#include "ex_texture.h"

VkDescriptorImageInfo
ex::texture::get_descriptor_info() {
    VkDescriptorImageInfo out_descriptor_image_info = {};
    out_descriptor_image_info.sampler = sampler;
    out_descriptor_image_info.imageView = image_view;
    out_descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    return out_descriptor_image_info;
}
