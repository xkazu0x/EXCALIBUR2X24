#include "ex_window.h"
#include "ex_logger.h"
#include "vk_backend.h"
#include <memory>
#include <chrono>

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

    vulkan_backend->create_swapchain(window->width(), window->height());
    vulkan_backend->create_render_pass();
    vulkan_backend->create_framebuffers();
    vulkan_backend->create_sync_structures();
    
    vulkan_backend->create_command_pool();    
    vulkan_backend->allocate_command_buffers();

    vulkan_backend->create_descriptor_set_layout();
    vulkan_backend->create_pipeline();

    std::vector<ex::vertex> vertices = {
        ex::vertex({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}),
        ex::vertex({ 0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}),
        ex::vertex({-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}),
        ex::vertex({ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}),
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        0, 3, 1,
    };
    
    vulkan_backend->create_vertex_buffer(vertices);
    vulkan_backend->create_index_buffer(indices);
    vulkan_backend->create_uniform_buffer();

    vulkan_backend->create_descriptor_pool();
    vulkan_backend->create_descriptor_set();

    auto last_time = std::chrono::high_resolution_clock::now();
    while (window->is_active()) {
        window->update();

        auto now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0f;        
        vulkan_backend->update(delta);
        
        if (!vulkan_backend->render()) {
            vulkan_backend->recreate_swapchain(window->width(), window->height());
            EXDEBUG("Width: %d | Height: %d", window->width(), window->height());
        }
    }
    
    vulkan_backend->shutdown();
    window->destroy();
    return 0;
}
