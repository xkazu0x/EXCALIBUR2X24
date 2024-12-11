#pragma once

#include "ex_window.h"
#include "ex_vertex.h"

#include "vk_image.h"
#include "vk_texture.h"
#include "vk_model.h"

#include "vk_shader.h"
#include "vk_pipeline.h"
#include "vk_buffer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <vector>
#include <memory>

namespace ex::vulkan {
    struct push_data {
        glm::mat4 transform;
    };
    
    struct uniform_data {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 light_pos;
    };
    
    class backend {
    public:
        bool initialize(ex::window *window);
        void shutdown();
        void begin_render();
        void end_render();
        
        void push_constant_data(ex::vulkan::pipeline *pipeline, ex::vulkan::push_data *push_data);
        void copy_buffer_data(ex::vulkan::buffer *buffer, const void *source, VkDeviceSize size);

        ex::vulkan::texture create_texture(const char *file);
        void destroy_texture(ex::vulkan::texture *texture);
        
        ex::vulkan::model create_model(const char *file);
        ex::vulkan::model create_model_from_array(std::vector<ex::vertex> &vertices, std::vector<uint32_t> &indices);
        void destroy_model(ex::vulkan::model *model);
        void draw_model(ex::vulkan::model *model);

        ex::vulkan::buffer create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        void destroy_buffer(ex::vulkan::buffer *buffer);

        VkDescriptorPool create_descriptor_pool(std::vector<VkDescriptorPoolSize> &pool_sizes);
        VkDescriptorSetLayout create_descriptor_set_layout(std::vector<VkDescriptorSetLayoutBinding> &bindings);
        void destroy_descriptor_pool(VkDescriptorPool *descriptor_pool);
        void destroy_descriptor_set_layout(VkDescriptorSetLayout *descriptor_set_layout);
        VkDescriptorSet allocate_descriptor_set(VkDescriptorPool *descriptor_pool, VkDescriptorSetLayout *descriptor_set_layout);
        void update_descriptor_sets(std::vector<VkWriteDescriptorSet> &write_descriptor_sets);
        
        ex::vulkan::pipeline create_pipeline(const char *vert_file, const char *frag_file, VkDescriptorSetLayout *descriptor_set_layout);
        void destroy_pipeline(ex::vulkan::pipeline *pipeline);
        void bind_pipeline(ex::vulkan::pipeline *pipeline, VkDescriptorSet *descriptor_set);
        
        float get_swapchain_aspect_ratio();
        
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
        
        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);        
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
