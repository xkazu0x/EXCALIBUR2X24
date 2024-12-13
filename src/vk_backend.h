#pragma once

#include "ex_window.h"
#include "ex_vertex.h"

#include "vk_image.h"
//#include "vk_model.h"
//#include "vk_texture.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <vector>
#include <memory>

namespace ex::vulkan {
    class backend {
    public:
        bool initialize(ex::window *window);
        void shutdown();
        void begin_render();
        void end_render();
        
        //ex::vulkan::texture create_texture(const char *file);
        //void destroy_texture(ex::vulkan::texture *texture);
        
        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);

        uint32_t get_memory_type_index(VkMemoryRequirements memory_requirements, VkMemoryPropertyFlags properties);
        float get_swapchain_aspect_ratio();
        VkCommandBuffer current_frame() { return m_command_buffers[m_next_image_index]; }
        
        VkAllocationCallbacks* allocator() { return m_allocator; }
        VkDevice& logical_device() { return m_logical_device; }
        VkExtent2D swapchain_extent() { return m_swapchain_extent; }
        VkRenderPass render_pass() { return m_render_pass; }
        uint32_t subpass() { return m_pipeline_subpass; }
        
    private:
        bool create_instance();
        void setup_debug_messenger();
        bool select_physical_device();
        bool create_logical_device();
        void create_command_pool();

        void create_swapchain(uint32_t width, uint32_t height);
        void recreate_swapchain(uint32_t width, uint32_t height);
        bool create_depth_resources();
        void create_render_pass();
        void create_framebuffers();
        void create_sync_structures();
        void allocate_command_buffers();
        
        VkImageView create_image_view(VkImage image, VkImageViewType type, VkFormat format, VkImageAspectFlags aspect_flags);

    private:
        ex::window *m_window;

        VkAllocationCallbacks *m_allocator;
        VkInstance m_instance;
#ifdef EXCALIBUR_DEBUG
        VkDebugUtilsMessengerEXT m_debug_messenger;
#endif
        VkSurfaceKHR m_surface;

        VkDevice m_logical_device;
        VkPhysicalDevice m_physical_device;
        uint32_t m_graphics_queue_index;
        uint32_t m_present_queue_index;
        VkQueue m_graphics_queue;
        VkQueue m_present_queue;

        VkCommandPool m_command_pool;
        std::vector<VkCommandBuffer> m_command_buffers;
        
        VkSurfaceCapabilitiesKHR m_swapchain_capabilities;
        std::vector<VkSurfaceFormatKHR> m_swapchain_formats;
        std::vector<VkPresentModeKHR> m_swapchain_present_modes;
        
        VkSwapchainKHR m_swapchain;
        VkSurfaceFormatKHR m_swapchain_format;
        VkPresentModeKHR m_swapchain_present_mode;
        VkExtent2D m_swapchain_extent;
        std::vector<VkImage> m_swapchain_images;
        std::vector<VkImageView> m_swapchain_image_views;
        std::vector<VkFramebuffer> m_swapchain_framebuffers;
        uint32_t m_next_image_index;

        ex::vulkan::image m_depth_image;
        VkRenderPass m_render_pass;

        VkFence m_fence;
        VkSemaphore m_semaphore_present;
        VkSemaphore m_semaphore_render;

        uint32_t m_pipeline_subpass;
    };
}
