#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace ex::vulkan {
    class backend {
    public:
        bool initialize();
        void shutdown();

    private:
        bool create_instance();
        void setup_debug_messenger();
        
    private:
        VkAllocationCallbacks *_allocator;
        VkInstance _instance;
#ifdef EXCALIBUR_DEBUG
        VkDebugUtilsMessengerEXT _debug_messenger;
#endif
    };
}
