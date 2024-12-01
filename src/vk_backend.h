#pragma once

#include "ex_window.h"
#include "ex_vertex.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>

namespace ex::vulkan {
    class backend {
    public:
        bool initialize(ex::window *window);
        void shutdown();
        void update(float delta);
        bool render();

        void create_command_pool();
        void create_swapchain(uint32_t width, uint32_t height);
        void create_render_pass();
        void create_framebuffers();
        void allocate_command_buffers();
        void create_sync_structures();

        bool create_depth_resources();
        
        void create_descriptor_set_layout();
        void create_pipeline();

        void create_texture_image(const char *file);

        void create_vertex_buffer(std::vector<ex::vertex> &vertices);
        void create_index_buffer(std::vector<uint32_t> &indices);
        void create_uniform_buffer();

        void create_descriptor_pool();
        void create_descriptor_set();
        
        void recreate_swapchain(uint32_t width, uint32_t height);

        VkDevice logical_device() {
            return m_logical_device;
        }

        VkAllocationCallbacks *allocator() {
            return m_allocator;
        }

    private:
        bool create_instance();
        void setup_debug_messenger();
        bool select_physical_device();
        bool create_logical_device();

        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);
        
        VkImageView create_image_view(VkImage image,
                                      VkImageViewType type,
                                      VkFormat format,
                                      VkImageAspectFlags aspect_flags);
        
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

        //std::vector<VkImage> m_depth_images;
        //std::vector<VkImageView> m_depth_image_views;
        //std::vector<VkDeviceMemory> m_depth_image_memories;

        VkFormat m_depth_format;
        VkImage m_depth_image;
        VkDeviceMemory m_depth_image_memory;
        VkImageView m_depth_image_view;
        
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

        VkImage m_texture_image;
        VkDeviceMemory m_texture_image_memory;
        VkImageLayout m_texture_image_layout;
        VkImageView m_texture_image_view;
        VkSampler m_texture_image_sampler;
        
        VkBuffer m_vertex_buffer;
        VkBuffer m_index_buffer;
        VkDeviceMemory m_vertex_buffer_memory;
        VkDeviceMemory m_index_buffer_memory;
        uint32_t m_vertex_count;
        uint32_t m_index_count;

        VkBuffer m_uniform_buffer;
        VkDeviceMemory m_uniform_buffer_memory;

        VkDescriptorSetLayout m_descriptor_set_layout;
        VkDescriptorPool m_descriptor_pool;
        VkDescriptorSet m_descriptor_set;
        
        glm::mat4 m_mvp;
    };
}
