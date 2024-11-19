#include "ex_window.h"
#include "ex_logger.h"

#include "vk_backend.h"

#include <memory>

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    const auto window = std::make_unique<ex::window>();
    const auto vulkan_backend = std::make_unique<ex::vulkan::backend>();
    
    window->create("EXCALIBUR", 960, 540);
    window->show();

    // VULKAN BACKEND
    if (!vulkan_backend->create_instance()) {
        EXFATAL("Failed to create vulkan instance");
        return -1;
    }
    vulkan_backend->setup_debug_messenger();
    if (!vulkan_backend->create_surface(&(*window))) {
        EXFATAL("Failed to create vulkan surface");
        return -1;
    }

    if (!vulkan_backend->select_physical_device()) {
        EXFATAL("Failed to select vulkan physical device");
        return -1;
    }
    
    if (!vulkan_backend->create_logical_device()) {
        EXFATAL("Failed to create vulkan logical device");
        return -1;
    }

    vulkan_backend->create_command_pool();
    vulkan_backend->create_swapchain(window->width(), window->height());
    vulkan_backend->create_render_pass();
    vulkan_backend->create_framebuffers();
    vulkan_backend->allocate_command_buffers();
    vulkan_backend->create_sync_structures();

    vulkan_backend->create_pipeline();
    
    while (window->is_active()) {
        window->update();
        vulkan_backend->render();
    }

    vulkan_backend->shutdown();
    window->destroy();
    return 0;
}
