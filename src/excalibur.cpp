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

    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(2);
    descriptor_set_layout_bindings[0].binding = 0;
    descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_layout_bindings[0].descriptorCount = 1;
    descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptor_set_layout_bindings[0].pImmutableSamplers = nullptr;

    descriptor_set_layout_bindings[1].binding = 1;
    descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_layout_bindings[1].descriptorCount = 1;
    descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_set_layout_bindings[1].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayout descriptor_set_layout = backend.create_descriptor_set_layout(descriptor_set_layout_bindings);
    ex::vulkan::pipeline graphics_pipeline = backend.create_pipeline("res/shaders/default.vert.spv",
                                                                     "res/shaders/default.frag.spv",
                                                                     &descriptor_set_layout);
    
    backend.create_uniform_buffer();
    backend.create_descriptor_pool();
    VkDescriptorImageInfo image_info = texture.get_descriptor_info();
    backend.create_descriptor_set(&descriptor_set_layout,
                                  &image_info);
    
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

    float light_speed = 0;
    auto last_time = std::chrono::high_resolution_clock::now();
    while (window.is_active()) {        
        window.update();

        auto now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration<float, std::chrono::seconds::period>(now - last_time).count();
        last_time = now;
        
        if (input.key_pressed(EX_KEY_ESCAPE)) window.close();
        if (input.key_pressed(EX_KEY_F1)) window.change_display_mode();
        
        if (input.button_pressed(EX_BUTTON_LEFT)) EXDEBUG("MOUSE LEFT BUTTON PRESSED");
        if (input.button_pressed(EX_BUTTON_RIGHT)) EXDEBUG("MOUSE RIGHT BUTTON PRESSED");
        if (input.button_pressed(EX_BUTTON_MIDDLE)) EXDEBUG("MOUSE MIDDLE BUTTON PRESSED");

        float speed = 2.5f * delta;
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
        light_speed += 100.0f * delta;
        uniform_data.light_pos = glm::vec3(0.0f, 3.0f, 6.0f);
        uniform_data.light_pos = glm::rotate(uniform_data.light_pos, glm::radians(light_speed) , glm::vec3(0.0f, 1.0f, 0.0f));
        backend.update_uniform_data(&uniform_data);

        float rotation_speed = 50.0f * delta;
        dragon_transform.rotation.y += rotation_speed;
        monkey_transform.rotation.y += rotation_speed;
        
        backend.begin_render();
        if (!window.is_minimized()) {
            backend.bind_pipeline(&graphics_pipeline);
            
            ex::vulkan::push_data push_data = {};
            push_data.transform = dragon_transform.matrix();
            backend.push_constant_data(&graphics_pipeline, &push_data);
            backend.draw_model(&dragon);

            push_data.transform = monkey_transform.matrix();
            backend.push_constant_data(&graphics_pipeline, &push_data);
            backend.draw_model(&monkey);
        }
        backend.end_render();
        input.update();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    backend.destroy_pipeline(&graphics_pipeline);
    backend.destroy_descriptor_set_layout(&descriptor_set_layout);
    
    backend.destroy_model(&monkey);
    backend.destroy_model(&dragon);
    backend.destroy_texture(&texture);
    
    backend.shutdown();
    window.destroy();
    input.shutdown();
    return 0;
}
