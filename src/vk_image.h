#pragma once

#include "vk_backend.h"

namespace ex::vulkan {
    class image {
    public:
        void set_type(VkImageType type);
        void set_format(VkFormat format);
        void set_extent(VkExtent2D extent);
        void set_tiling(VkImageTiling tiling);
        void set_usage(VkImageUsageFlags usage);
        void set_layout(VkImageLayout layout);
        void create(ex::vulkan::backend *backend);
        void destroy(ex::vulkan::backend *backend);

        void bind(ex::vulkan::backend *backend);
        void change_layout(VkCommandBuffer command_buffer, VkImageLayout layout, VkImageAspectFlags aspect_mask);
        void copy_buffer_to(VkCommandBuffer command_buffer, VkBuffer buffer, VkImageAspectFlags aspect_mask, VkExtent2D extent);
        void create_view(ex::vulkan::backend *backend, VkImageViewType view_type, VkImageAspectFlags aspect_flags);

        VkImage handle() { return m_handle; }
        VkImageView view() { return m_view; }
        VkFormat format() { return m_format; }
        
    private:
        VkImage m_handle;
        VkDeviceMemory m_memory;
        VkImageView m_view;
        VkImageType m_type;
        VkFormat m_format;
        VkExtent2D m_extent;
        VkImageTiling m_tiling;
        VkImageUsageFlags m_usage;
        VkImageLayout m_layout;
    };
}
