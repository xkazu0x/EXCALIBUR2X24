#pragma once

#include "vk_image.h"
#include <vulkan/vulkan.h>

namespace ex::vulkan {
    class texture {
    public:
        VkDescriptorImageInfo *get_descriptor_info();
        
    public:
        ex::vulkan::image image;
        VkSampler sampler;
        VkDescriptorImageInfo m_descriptor_info;
    };
}
