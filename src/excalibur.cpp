#include "ex_logger.h"
#include "ex_window.h"
#include "ex_input.h"
#include "vk_backend.h"

#include <memory>
#include <chrono>
#include <thread>

ex::window window;
ex::input input;
ex::vulkan::backend backend;

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
    
    if (!backend.initialize(&window)) return -1;
    backend.create_descriptor_set_layout();
    backend.create_pipeline();
    backend.create_texture_image("res/textures/paris.jpg");
    backend.create_models();
    backend.create_uniform_buffer();
    backend.create_descriptor_pool();
    backend.create_descriptor_set();

    auto last_time = std::chrono::high_resolution_clock::now();
    while (window.is_active()) {
        window.update();
        input.update(&window);
        key_presses();

        auto now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() / 1000.0f;
        backend.update(delta);
        
        backend.begin();
        backend.render();
        backend.end();
                
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    backend.shutdown();
    window.destroy();
    return 0;
}
