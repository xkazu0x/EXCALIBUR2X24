#pragma once

#include "ex_window.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vector>

namespace ex {
    struct vec3 {
        float x;
        float y;
        float z;
    };
    
    class vertex {
    public:
        vertex(vec3 pos, vec3 color)
            : pos(pos), color(color) {
        }
        
    public:
        vec3 pos;
        vec3 color;
    };
}

namespace ex::vulkan {
    class backend {
    public:
        void shutdown();
        bool render();

        bool create_instance();
        void setup_debug_messenger();
        bool create_surface(ex::window *window);
        bool select_physical_device();
        bool create_logical_device();
        void create_command_pool();
        void create_swapchain(uint32_t width, uint32_t height);
        void create_render_pass();
        void create_framebuffers();
        void allocate_command_buffers();
        void create_sync_structures();
        void create_pipeline();
        void create_vertex_buffer(std::vector<ex::vertex> &vertices);
        void create_index_buffer(std::vector<uint32_t> &indices);
        
        void recreate_swapchain(uint32_t width, uint32_t height);

        VkDevice logical_device() {
            return m_logical_device;
        }

    private:
        std::vector<char> read_file(const char *filepath);
        VkShaderModule create_shader_module(const std::vector<char> &shader_module);
        void create_buffer(VkDeviceSize size,
                           VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags properties,
                           VkBuffer &buffer,
                           VkDeviceMemory &buffer_memory);
        
    private:
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
        
        VkSwapchainKHR m_swapchain;
        VkSurfaceFormatKHR m_swapchain_format;
        VkPresentModeKHR m_swapchain_present_mode;
        VkExtent2D m_swapchain_extent;
        std::vector<VkImage> m_swapchain_images;
        std::vector<VkImageView> m_swapchain_image_views;
        std::vector<VkFramebuffer> m_swapchain_framebuffers;

        std::vector<VkImage> m_depth_images;
        std::vector<VkImageView> m_depth_image_views;
        std::vector<VkDeviceMemory> m_depth_image_memories;
        
        VkSurfaceCapabilitiesKHR m_swapchain_capabilities;
        std::vector<VkSurfaceFormatKHR> m_swapchain_formats;
        std::vector<VkPresentModeKHR> m_swapchain_present_modes;

        VkRenderPass m_render_pass;

        VkFence m_fence;
        VkSemaphore m_semaphore_present;
        VkSemaphore m_semaphore_render;

        VkPipeline m_graphics_pipeline;
        VkViewport m_pipeline_viewport;
        VkRect2D m_pipeline_scissor;
        VkPipelineLayout m_pipeline_layout;
        uint32_t m_pipeline_subpass;

        VkBuffer m_vertex_buffer;
        VkBuffer m_index_buffer;
        VkDeviceMemory m_vertex_buffer_memory;
        VkDeviceMemory m_index_buffer_memory;
        uint32_t m_vertex_count;
        uint32_t m_index_count;
    };
}
