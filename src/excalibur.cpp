#include "ex_logger.h"
#include "ex_window.h"
#include "ex_input.h"
#include "ex_component.hpp"
#include "ex_camera.h"

#include "vk_backend.h"
#include "vk_shader.h"
#include "vk_descriptor.h"
#include "vk_pipeline.h"

#include <memory>
#include <chrono>
#include <thread>

ex::input input;
ex::window window;
ex::vulkan::backend backend;

namespace ex {
    struct object {
        ex::vulkan::model model;
        ex::comp::transform transform;
    };

    struct push_constants {
        glm::mat4 transform;
    };
}

struct objects {
    ex::object dragon;
    ex::object monkey;
    ex::object grid;
} objects;

struct vulkan_shaders {
    ex::vulkan::shader colored;
    ex::vulkan::shader textured;
} shaders;

struct vulkan_descriptor_sets {
    ex::vulkan::descriptor_set uniform;
    ex::vulkan::descriptor_set textures;
} descriptor_sets;

struct vulkan_pipelines {
    ex::vulkan::pipeline colored;
    ex::vulkan::pipeline textured;
} pipelines;

void create_grid(uint32_t size, std::vector<ex::vertex> *vertices, std::vector<uint32_t> *indices);

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    input.initialize();

    window.create(&input, "EXCALIBUR", 960, 540, false);
    window.show();

    if (!backend.initialize(&window)) {
        EXFATAL("Failed to initialize vulkan backend");
        return -1;
    }
    
    objects.dragon.model = backend.create_model("res/meshes/dragon.obj");
    objects.dragon.transform.translation = glm::vec3(-1.5f, 0.0f, 0.0f);
    objects.dragon.transform.rotation = glm::vec3(0.0f, -60.0f, 0.0f);
    objects.dragon.transform.scale = glm::vec3(0.25f);
    EXDEBUG("Dragon mesh vertex count: %d", objects.dragon.model.vertex_count());
    EXDEBUG("Dragon mesh index count: %d", objects.dragon.model.index_count());
    
    objects.monkey.model = backend.create_model("res/meshes/monkey_smooth.obj");
    objects.monkey.transform.translation = glm::vec3(1.5f, 1.0f, 0.0f);
    objects.monkey.transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    objects.monkey.transform.scale = glm::vec3(1.0f);
    EXDEBUG("Monkey mesh vertex count: %d", objects.monkey.model.vertex_count());
    EXDEBUG("Monkey mesh index count: %d", objects.monkey.model.index_count());

    std::vector<ex::vertex> grid_vertices;
    std::vector<uint32_t> grid_indices;
    create_grid(10, &grid_vertices, &grid_indices);
    objects.grid.model = backend.create_model_from_array(grid_vertices, grid_indices);
    objects.grid.transform.translation = glm::vec3(-5.0f, 0.0f, -5.0f);
    objects.grid.transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    objects.grid.transform.scale = glm::vec3(1.0f);
    EXDEBUG("Grid mesh vertex count: %d", objects.grid.model.vertex_count());
    EXDEBUG("Grid mesh index count: %d", objects.grid.model.index_count());
    
    ex::vulkan::texture texture = backend.create_texture("res/textures/paris.jpg");
    ex::vulkan::buffer uniform_buffer = backend.create_buffer(sizeof(ex::vulkan::uniform_data),
                                                              VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    ex::vulkan::descriptor_pool descriptor_pool;
    descriptor_pool.add_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
    descriptor_pool.add_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
    descriptor_pool.create(&backend, 2);
    
    descriptor_sets.uniform.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
    descriptor_sets.uniform.build_layout(&backend);
    descriptor_sets.uniform.allocate(&backend, &descriptor_pool);
    descriptor_sets.uniform.add_write_set(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, uniform_buffer.get_descriptor_info());
    descriptor_sets.uniform.update(&backend);

    descriptor_sets.textures.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
    descriptor_sets.textures.build_layout(&backend);
    descriptor_sets.textures.allocate(&backend, &descriptor_pool);
    descriptor_sets.textures.add_write_set(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texture.get_descriptor_info(), nullptr);
    descriptor_sets.textures.update(&backend);

    shaders.colored.create(&backend, "res/shaders/default.vert.spv", "res/shaders/colored.frag.spv");
    shaders.textured.create(&backend, "res/shaders/default.vert.spv", "res/shaders/textured.frag.spv");

    pipelines.colored.push_descriptor_set_layout(descriptor_sets.uniform.layout());
    pipelines.colored.set_push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ex::push_constants));
    pipelines.colored.build_layout(&backend);
    pipelines.colored.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelines.colored.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelines.colored.set_cull_mode(VK_CULL_MODE_BACK_BIT);
    pipelines.colored.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    pipelines.colored.build(&backend, &shaders.colored);

    pipelines.textured.push_descriptor_set_layout(descriptor_sets.uniform.layout());
    pipelines.textured.push_descriptor_set_layout(descriptor_sets.textures.layout());
    pipelines.textured.set_push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ex::push_constants));
    pipelines.textured.build_layout(&backend);
    pipelines.textured.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelines.textured.set_polygon_mode(VK_POLYGON_MODE_FILL);
    pipelines.textured.set_cull_mode(VK_CULL_MODE_BACK_BIT);
    pipelines.textured.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    pipelines.textured.build(&backend, &shaders.textured);

    shaders.colored.destroy(&backend);
    shaders.textured.destroy(&backend);
    
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
        if (input.key_down(EX_KEY_A)) camera.translate({speed, 0.0f, 0.0f});
        if (input.key_down(EX_KEY_D)) camera.translate({-speed, 0.0f, 0.0f});
        
        if (input.key_down(EX_KEY_LEFT)) objects.monkey.transform.translation -= glm::vec3(speed, 0.0f, 0.0f);
        if (input.key_down(EX_KEY_RIGHT)) objects.monkey.transform.translation += glm::vec3(speed, 0.0f, 0.0f);
        if (input.key_down(EX_KEY_UP)) objects.monkey.transform.translation += glm::vec3(0.0f, 0.0f, speed);
        if (input.key_down(EX_KEY_DOWN)) objects.monkey.transform.translation -= glm::vec3(0.0f, 0.0f, speed);

        camera.update_aspect_ratio(backend.get_swapchain_aspect_ratio());

        ex::vulkan::uniform_data uniform_data = {};
        uniform_data.view = camera.get_view();
        uniform_data.projection = camera.get_projection();
        light_speed += 100.0f * delta;
        uniform_data.light_pos = glm::vec3(0.0f, 3.0f, 6.0f);
        uniform_data.light_pos = glm::rotate(uniform_data.light_pos, glm::radians(light_speed) , glm::vec3(0.0f, 1.0f, 0.0f));
        backend.copy_buffer_data(&uniform_buffer, &uniform_data, sizeof(ex::vulkan::uniform_data));

        float rotation_speed = 50.0f * delta;
        objects.dragon.transform.rotation.y += rotation_speed;
        objects.monkey.transform.rotation.y += rotation_speed;
        //objects.grid.transform.rotation.y += rotation_speed;
        
        backend.begin_render();
        if (!window.is_minimized()) {
            pipelines.textured.bind(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            pipelines.textured.update_dynamic(backend.current_frame(), backend.swapchain_extent());
            pipelines.textured.bind_descriptor_sets(backend.current_frame(),
                                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                    {descriptor_sets.uniform.handle(),
                                                     descriptor_sets.textures.handle()});
            
            ex::push_constants constants = {};
            constants.transform = objects.dragon.transform.matrix();
            pipelines.textured.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT, &constants);
            backend.draw_model(&objects.dragon.model);

            std::vector<VkDescriptorSet> sets = {descriptor_sets.uniform.handle()};
            pipelines.colored.bind(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            pipelines.colored.update_dynamic(backend.current_frame(), backend.swapchain_extent());
            pipelines.colored.bind_descriptor_sets(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
            
            constants.transform = objects.monkey.transform.matrix();
            pipelines.colored.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT, &constants);
            backend.draw_model(&objects.monkey.model);

            constants.transform = objects.grid.transform.matrix();
            pipelines.colored.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT, &constants);
            backend.draw_model(&objects.grid.model);
        }
        backend.end_render();
        input.update();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    pipelines.textured.destroy(&backend);
    pipelines.colored.destroy(&backend);
    
    descriptor_sets.textures.destroy_layout(&backend);
    descriptor_sets.uniform.destroy_layout(&backend);
    descriptor_pool.destroy(&backend);

    backend.destroy_buffer(&uniform_buffer);
    backend.destroy_texture(&texture);
    
    backend.destroy_model(&objects.grid.model);
    backend.destroy_model(&objects.monkey.model);
    backend.destroy_model(&objects.dragon.model);
    
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
