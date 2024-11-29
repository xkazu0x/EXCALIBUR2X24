#include "ex_logger.h"
#include "ex_window.h"
#include "ex_input.h"
#include "vk_backend.h"
#include <memory>
#include <chrono>

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    ex::window window;
    window.create("EXCALIBUR", 960, 540);
    window.show();
    
    const auto vulkan_backend = std::make_unique<ex::vulkan::backend>();
    if (!vulkan_backend->initialize(&window)) {
        EXFATAL("Failed to initialize vulkan backend");
        return -1;
    }

    vulkan_backend->create_swapchain(window.width(), window.height());
    vulkan_backend->create_render_pass();
    vulkan_backend->create_framebuffers();
    vulkan_backend->create_sync_structures();

    //vulkan_backend->create_depth_resources();
    
    vulkan_backend->create_command_pool();    
    vulkan_backend->allocate_command_buffers();

    vulkan_backend->create_descriptor_set_layout();
    vulkan_backend->create_pipeline();

    vulkan_backend->create_texture_image("res/textures/paris.jpg");

    /*
    std::vector<ex::vertex> vertices = {
        // FRONT FACE
        ex::vertex({-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}),
        ex::vertex({ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}),
        ex::vertex({-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}),
        ex::vertex({ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}),

        // BACK FACE
        ex::vertex({-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}),
        ex::vertex({ 0.5f,  0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}),
        ex::vertex({-0.5f,  0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}),
        ex::vertex({ 0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}),
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        0, 3, 1,

        4, 5, 6,
        4, 7, 5,

        4, 3, 0,
        4, 7, 3,

        6, 1, 2,
        6, 5, 1,
    };
    */

    std::vector<ex::vertex> vertices = {
        ex::vertex({-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}),
        ex::vertex({ 0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}),
        ex::vertex({-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}),
        ex::vertex({ 0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}),
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

    ex::input input;
    auto last_time = std::chrono::high_resolution_clock::now();
    while (window.is_active()) {
        window.update();
        input.update(&window);
        
        if (input.key_pressed(VK_ESCAPE)) {
            window.close();
        }
        
        auto now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0f;
        vulkan_backend->update(delta);
        
        if (!vulkan_backend->render()) {
            vulkan_backend->recreate_swapchain(window.width(), window.height());
            EXDEBUG("Width: %d | Height: %d", window.width(), window.height());
        }
    }
    
    vulkan_backend->shutdown();
    window.destroy();
    return 0;
}
