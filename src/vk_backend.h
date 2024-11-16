#pragma once

#include "ex_window.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace ex::vulkan {
    class backend {
    public:
        void shutdown();

        bool create_instance();
        void setup_debug_messenger();
        bool create_surface(ex::window *window);
        bool select_physical_device();
        bool create_logical_device();
        
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
    };
}
