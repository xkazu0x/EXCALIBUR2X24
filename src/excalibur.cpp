#include "ex_logger.h"
#include "ex_window.h"
#include "ex_input.h"
#include "vk_backend.h"

#include <memory>
#include <chrono>
#include <thread>

ex::window window;
ex::vulkan::backend backend;
ex::input input;

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    window.create("EXCALIBUR", 960, 540, false);
    window.show();

    if (!backend.initialize(&window)) return -1;
    ex::texture texture = backend.create_texture("res/textures/paris.jpg");
    ex::model dragon = backend.create_model("res/meshes/dragon.obj");
    ex::model monkey = backend.create_model("res/meshes/monkey.obj");
    
    backend.create_descriptor_set_layout();
    backend.create_graphics_pipeline();    
    backend.create_uniform_buffer();
    backend.create_descriptor_pool();

    VkDescriptorImageInfo image_info = texture.get_descriptor_info();
    backend.create_descriptor_set(&image_info);

    auto last_time = std::chrono::high_resolution_clock::now();
    while (window.is_active()) {
        window.update();
        input.update(&window);
        
        if (input.key_pressed(VK_ESCAPE)) window.close();
        if (input.key_pressed(VK_F1)) window.change_display_mode();

        auto now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0f;
        backend.update(delta);
        
        backend.begin_render();
        backend.bind_pipeline();
        
        ex::vulkan::push_data push_data = {};
        push_data.transform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, 0.0f));
        push_data.transform = glm::rotate(push_data.transform, glm::radians(30.0f) * delta, glm::vec3(0.0f, 1.0f, 0.0f));
        push_data.transform = glm::scale(push_data.transform, glm::vec3(0.2f));
        backend.push_constant_data(&push_data);
        backend.draw_model(&dragon);

        push_data.transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 1.0f, 0.0f));
        push_data.transform = glm::rotate(push_data.transform, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        push_data.transform = glm::rotate(push_data.transform, glm::radians(30.0f) * delta, glm::vec3(0.0f, 1.0f, 0.0f));
        push_data.transform = glm::scale(push_data.transform, glm::vec3(0.8f));
        backend.push_constant_data(&push_data);
        backend.draw_model(&monkey);
        
        backend.end_render();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    backend.destroy_model(&monkey);
    backend.destroy_model(&dragon);
    backend.destroy_texture(&texture);
    
    backend.shutdown();
    window.destroy();
    return 0;
}
