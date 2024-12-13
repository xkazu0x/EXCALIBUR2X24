#pragma once

#include "vk_buffer.h"
#include "vk_image.h"

namespace ex::vulkan {
    class texture {        
    public:
        void create(ex::vulkan::backend *backend, const char *file_path);
        void destroy(ex::vulkan::backend *backend);
        
        VkDescriptorImageInfo *get_descriptor_info();
        
    private:
        ex::vulkan::image m_image;
        VkSampler m_sampler;
        VkDescriptorImageInfo m_descriptor_info;
    };
}
