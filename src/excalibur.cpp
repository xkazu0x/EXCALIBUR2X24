#include "ex_logger.h"
#include "ex_window.h"
#include "ex_input.h"
#include "ex_mesh.h"

#include "vk_backend.h"

#include <tinyobjloader/tiny_obj_loader.h>

#include <memory>
#include <chrono>
#include <thread>

ex::window window;
ex::input input;
const auto vulkan_backend = std::make_unique<ex::vulkan::backend>();

void key_presses() {
    if (input.key_pressed(VK_ESCAPE)) {
        window.close();
    }
    if (input.key_pressed(VK_F1)) {
        window.change_display_mode();
    }
}

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    window.create("EXCALIBUR", 960, 540, false);
    window.show();
    
    if (!vulkan_backend->initialize(&window)) {
        EXFATAL("Failed to initialize vulkan backend");
        return -1;
    }

    vulkan_backend->create_swapchain(window.width(), window.height());
    if (!vulkan_backend->create_depth_resources()) return -1;    
    vulkan_backend->create_render_pass();
    vulkan_backend->create_framebuffers();
    vulkan_backend->create_sync_structures();
    
    vulkan_backend->create_command_pool();
    vulkan_backend->allocate_command_buffers();
    
    vulkan_backend->create_descriptor_set_layout();
    vulkan_backend->create_pipeline();

    vulkan_backend->create_texture_image("res/textures/paris.jpg");

    ex::mesh mesh;
    mesh.create("res/meshes/dragon.obj");

    vulkan_backend->create_vertex_buffer(mesh.vertices());
    vulkan_backend->create_index_buffer(mesh.indices());
    vulkan_backend->create_uniform_buffer();

    vulkan_backend->create_descriptor_pool();
    vulkan_backend->create_descriptor_set();
    
    ex::vulkan::ubo ubo;
    
    auto last_time = std::chrono::high_resolution_clock::now();
    while (window.is_active()) {
        window.update();
        input.update(&window);
        key_presses();

        auto now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0f;

        // update
        ubo.model = glm::mat4(1.0f);
        //ubo.model = glm::rotate(ubo.model, glm::radians(30.0f) * delta, glm::vec3(0.0f, 1.0f, 0.0f));

        float scale = 0.25f;
        ubo.model = glm::scale(ubo.model, glm::vec3(scale, scale, scale));

        ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f),
                               glm::vec3(0.0f, -1.0f, 0.0f));
        ubo.view = glm::rotate(ubo.view, glm::radians(-20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ubo.view = glm::translate(ubo.view, glm::vec3(0.0f, -2.2f, 3.0f));
        ubo.view = glm::rotate(ubo.view, glm::radians(30.0f) * delta, glm::vec3(0.0f, 1.0f, 0.0f));
        
        float aspect = vulkan_backend->swapchain_extent().width / (float) vulkan_backend->swapchain_extent().height;
        ubo.projection = glm::perspective(glm::radians(80.0f),
                                          aspect,
                                          0.01f,
                                          10.0f);

        ubo.light_pos = glm::vec3(0.0f, 3.0f, 6.0f);
        ubo.light_pos = glm::rotate(ubo.light_pos, glm::radians(60.0f) * delta, glm::vec3(0.0f, 1.0f, 0.0f));
        
        vulkan_backend->upload_uniform_buffer(&ubo);
        
        if (!vulkan_backend->render()) {
            vulkan_backend->recreate_swapchain(window.width(), window.height());
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    vulkan_backend->shutdown();
    window.destroy();
    return 0;
}
