#pragma once

#include "vk_image.h"
#include <vulkan/vulkan.h>

namespace ex {
    struct texture {
        ex::vulkan::image image;
        VkSampler sampler;

        VkDescriptorImageInfo get_descriptor_info();
    };
}
