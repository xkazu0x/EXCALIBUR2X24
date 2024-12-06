#pragma once

#include <vulkan/vulkan.h>

namespace ex::vulkan {
    class image {
    public:
        void create(VkDevice logical_device,
                    VkPhysicalDevice physical_device,
                    VkAllocationCallbacks *allocator,
                    VkImageType type,
                    VkFormat format,
                    VkExtent2D extent,
                    VkImageTiling tiling,
                    VkImageUsageFlags usage,
                    VkImageLayout layout);
        void destroy(VkDevice logical_device,
                     VkAllocationCallbacks *allocator);
        void change_layout(VkCommandBuffer command_buffer,
                           VkImageLayout new_layout,
                           VkImageAspectFlags aspect_mask);
        void copy_buffer(VkCommandBuffer command_buffer,
                         VkBuffer src_buffer,
                         VkImageAspectFlags aspect_mask,
                         VkExtent2D extent);

        VkImageView create_view(VkDevice logical_device,
                                VkAllocationCallbacks *allocator,
                                VkImageViewType view_type,
                                VkImageAspectFlags aspect_flags);

        VkImage handle() {
            return m_handle;
        }

        VkFormat format() {
            return m_format;
        }
        
    private:
        VkImage m_handle;
        VkDeviceMemory m_memory;
        VkFormat m_format;
        VkImageLayout m_layout;
    };
}
