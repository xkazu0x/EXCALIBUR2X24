#pragma once

#include "vk_backend.h"
#include "vk_buffer.h"
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
