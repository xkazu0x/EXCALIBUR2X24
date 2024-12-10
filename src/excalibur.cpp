#include "ex_logger.h"
#include "ex_window.h"
#include "ex_input.h"
#include "ex_component.hpp"
#include "ex_camera.h"
#include "vk_backend.h"

#include <memory>
#include <chrono>
#include <thread>

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    ex::input input = {};
    input.initialize();

    ex::window window = {};
    window.create(&input, "EXCALIBUR", 960, 540, false);
    window.show();

    ex::vulkan::backend backend = {};
    if (!backend.initialize(&window)) {
        EXFATAL("Failed to initialize vulkan backend");
        return -1;
    }
    
    ex::vulkan::texture texture = backend.create_texture("res/textures/paris.jpg");
    ex::vulkan::model dragon = backend.create_model("res/meshes/dragon.obj");
    ex::vulkan::model monkey = backend.create_model("res/meshes/monkey.obj");
    
    backend.create_descriptor_set_layout();
    backend.create_graphics_pipeline();    
    backend.create_uniform_buffer();
    backend.create_descriptor_pool();

    VkDescriptorImageInfo image_info = texture.get_descriptor_info();
    backend.create_descriptor_set(&image_info);
    
    ex::comp::transform dragon_transform = {};
    dragon_transform.translation = glm::vec3(-1.5f, 0.0f, 0.0f);
    dragon_transform.rotation = glm::vec3(0.0f, -60.0f, 0.0f);
    dragon_transform.scale = glm::vec3(0.2f);
    
    ex::comp::transform monkey_transform = {};
    monkey_transform.translation = glm::vec3(1.5f, 1.0f, 0.0f);
    monkey_transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    monkey_transform.scale = glm::vec3(0.8f);   

    ex::camera camera = {};
    camera.set_translation(glm::vec3(0.0f, -1.8f, 3.0f));
    camera.set_rotation(glm::vec3(-20.0f, 0.0f, 0.0f));
    camera.set_perspective(80.0f, (float) window.width() / (float) window.height(), 0.1f, 10.0f);
    
    auto last_time = std::chrono::high_resolution_clock::now();
    while (window.is_active()) {
        auto now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0f;
        
        window.update();
        
        if (input.key_pressed(EX_KEY_ESCAPE)) window.close();
        if (input.key_pressed(EX_KEY_F1)) window.change_display_mode();
        
        if (input.button_pressed(EX_BUTTON_LEFT)) EXDEBUG("MOUSE LEFT BUTTON PRESSED");
        if (input.button_pressed(EX_BUTTON_RIGHT)) EXDEBUG("MOUSE RIGHT BUTTON PRESSED");
        if (input.button_pressed(EX_BUTTON_MIDDLE)) EXDEBUG("MOUSE MIDDLE BUTTON PRESSED");

        float speed = 0.1f;
        if (input.key_down(EX_KEY_LEFT))
            monkey_transform.translation -= glm::vec3(speed, 0.0f, 0.0f);
        if (input.key_down(EX_KEY_RIGHT))
            monkey_transform.translation += glm::vec3(speed, 0.0f, 0.0f);
        if (input.key_down(EX_KEY_UP))
            monkey_transform.translation += glm::vec3(0.0f, 0.0f, speed);
        if (input.key_down(EX_KEY_DOWN))
            monkey_transform.translation -= glm::vec3(0.0f, 0.0f, speed);

        camera.update_aspect_ratio(backend.get_swapchain_aspect_ratio());
        
        ex::vulkan::uniform_data uniform_data = {};
        uniform_data.view = camera.get_view();
        uniform_data.projection = camera.get_projection();        
        uniform_data.light_pos = glm::vec3(0.0f, 3.0f, 6.0f);
        uniform_data.light_pos = glm::rotate(uniform_data.light_pos, glm::radians(60.0f) * delta, glm::vec3(0.0f, 1.0f, 0.0f));
        backend.update_uniform_data(&uniform_data);

        backend.begin_render();
        if (!window.is_minimized()) {
            backend.bind_pipeline();
            
            ex::vulkan::push_data push_data = {};
            push_data.transform = dragon_transform.matrix();
            backend.push_constant_data(&push_data);
            backend.draw_model(&dragon);

            push_data.transform = monkey_transform.matrix();
            backend.push_constant_data(&push_data);
            backend.draw_model(&monkey);
        }
        backend.end_render();
        input.update();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    backend.destroy_model(&monkey);
    backend.destroy_model(&dragon);
    backend.destroy_texture(&texture);
    
    backend.shutdown();
    window.destroy();
    input.shutdown();
    return 0;
}
