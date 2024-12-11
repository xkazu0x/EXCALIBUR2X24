#include "ex_logger.h"
#include "ex_window.h"
#include "ex_input.h"
#include "ex_component.hpp"
#include "ex_camera.h"
#include "vk_backend.h"

#include <memory>
#include <chrono>
#include <thread>

namespace ex {
    struct object {
        ex::vulkan::model model;
        ex::comp::transform transform;
    };
}

void create_grid(uint32_t size, std::vector<ex::vertex> *vertices, std::vector<uint32_t> *indices);

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
    
    ex::object dragon = {};
    dragon.model = backend.create_model("res/meshes/dragon.obj");
    EXDEBUG("Dragon mesh vertex count: %d", dragon.model.vertex_count());
    EXDEBUG("Dragon mesh index count: %d", dragon.model.index_count());
    dragon.transform.translation = glm::vec3(-1.5f, 0.0f, 0.0f);
    dragon.transform.rotation = glm::vec3(0.0f, -60.0f, 0.0f);
    dragon.transform.scale = glm::vec3(0.25f);
    
    ex::object monkey = {};
    monkey.model = backend.create_model("res/meshes/monkey_smooth.obj");
    EXDEBUG("Monkey mesh vertex count: %d", monkey.model.vertex_count());
    EXDEBUG("Monkey mesh index count: %d", monkey.model.index_count());
    monkey.transform.translation = glm::vec3(1.5f, 1.0f, 0.0f);
    monkey.transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    monkey.transform.scale = glm::vec3(1.0f);
    
    ex::object grid = {};
    std::vector<ex::vertex> grid_vertices = {};
    std::vector<uint32_t> grid_indices = {};
    create_grid(10, &grid_vertices, &grid_indices);
    grid.model = backend.create_model_from_array(grid_vertices, grid_indices);
    EXDEBUG("Grid mesh vertex count: %d", grid.model.vertex_count());
    EXDEBUG("Grid mesh index count: %d", grid.model.index_count());
    grid.transform.translation = glm::vec3(-5.0f, 0.0f, -5.0f);
    grid.transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    grid.transform.scale = glm::vec3(1.0f);

    ex::vulkan::buffer uniform_buffer = backend.create_buffer(sizeof(ex::vulkan::uniform_data),
                                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
    descriptor_pool_sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
    descriptor_pool_sizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
    VkDescriptorPool descriptor_pool = backend.create_descriptor_pool(descriptor_pool_sizes);    
    std::vector<VkDescriptorSetLayoutBinding> descriptor_bindings;
    descriptor_bindings.push_back({0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr});
    descriptor_bindings.push_back({1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr});
    VkDescriptorSetLayout descriptor_set_layout = backend.create_descriptor_set_layout(descriptor_bindings);
    
    VkDescriptorSet descriptor_set = backend.allocate_descriptor_set(&descriptor_pool, &descriptor_set_layout);    
    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    write_descriptor_sets.push_back({
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_set,
            0,
            0,
            1,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            nullptr,
            uniform_buffer.get_descriptor_info(),
            nullptr});
    write_descriptor_sets.push_back({
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            descriptor_set,
            1,
            0,
            1,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            texture.get_descriptor_info(),
            nullptr,
            nullptr});
    backend.update_descriptor_sets(write_descriptor_sets);
    
    ex::vulkan::pipeline graphics_pipeline = backend.create_pipeline("res/shaders/default.vert.spv",
                                                                     "res/shaders/default.frag.spv",
                                                                     &descriptor_set_layout);
            
    ex::camera camera = {};
    camera.set_translation(glm::vec3(0.0f, -2.0f, 4.0f));
    camera.set_rotation(glm::vec3(-15.0f, 0.0f, 0.0f));
    camera.set_perspective(80.0f, (float) window.width() / (float) window.height(), 0.1f, 20.0f);

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
            monkey.transform.translation -= glm::vec3(speed, 0.0f, 0.0f);
        if (input.key_down(EX_KEY_RIGHT))
            monkey.transform.translation += glm::vec3(speed, 0.0f, 0.0f);
        if (input.key_down(EX_KEY_UP))
            monkey.transform.translation += glm::vec3(0.0f, 0.0f, speed);
        if (input.key_down(EX_KEY_DOWN))
            monkey.transform.translation -= glm::vec3(0.0f, 0.0f, speed);

        camera.update_aspect_ratio(backend.get_swapchain_aspect_ratio());

        ex::vulkan::uniform_data uniform_data = {};
        uniform_data.view = camera.get_view();
        uniform_data.projection = camera.get_projection();
        light_speed += 100.0f * delta;
        uniform_data.light_pos = glm::vec3(0.0f, 3.0f, 6.0f);
        uniform_data.light_pos = glm::rotate(uniform_data.light_pos, glm::radians(light_speed) , glm::vec3(0.0f, 1.0f, 0.0f));
        backend.copy_buffer_data(&uniform_buffer, &uniform_data, sizeof(ex::vulkan::uniform_data));

        float rotation_speed = 50.0f * delta;
        dragon.transform.rotation.y += rotation_speed;
        monkey.transform.rotation.y += rotation_speed;
        
        backend.begin_render();
        if (!window.is_minimized()) {
            backend.bind_pipeline(&graphics_pipeline, &descriptor_set);
            
            ex::vulkan::push_data push_data = {};
            push_data.transform = grid.transform.matrix();
            backend.push_constant_data(&graphics_pipeline, &push_data);
            backend.draw_model(&grid.model);
            
            push_data.transform = dragon.transform.matrix();
            backend.push_constant_data(&graphics_pipeline, &push_data);
            backend.draw_model(&dragon.model);

            push_data.transform = monkey.transform.matrix();
            backend.push_constant_data(&graphics_pipeline, &push_data);
            backend.draw_model(&monkey.model);
        }
        backend.end_render();
        input.update();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    backend.destroy_pipeline(&graphics_pipeline);
    backend.destroy_descriptor_set_layout(&descriptor_set_layout);
    backend.destroy_descriptor_pool(&descriptor_pool);

    backend.destroy_buffer(&uniform_buffer);
    
    backend.destroy_model(&grid.model);
    backend.destroy_model(&monkey.model);
    backend.destroy_model(&dragon.model);
    backend.destroy_texture(&texture);
    
    backend.shutdown();
    window.destroy();
    input.shutdown();
    return 0;
}

void create_grid(uint32_t size,
                 std::vector<ex::vertex> *vertices,
                 std::vector<uint32_t> *indices) {
    for (uint32_t z = 0; z < size; z++) {
        for (uint32_t x = 0; x < size; x++) {
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 1.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 0.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 0.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 1.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 0.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 1.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           {0.0f, 0.0f, 0.0f}));
            indices->push_back(indices->size());
            indices->push_back(indices->size());
            indices->push_back(indices->size());
            indices->push_back(indices->size());
            indices->push_back(indices->size());
            indices->push_back(indices->size());
        }
    }
}
