#include "ex_logger.h"
#include "ex_window.h"
#include "ex_input.h"
#include "ex_component.hpp"
#include "ex_camera.h"
#include "ex_mesh.h"

#include "vk_backend.h"
#include "vk_model.h"
#include "vk_texture.h"
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

    struct ubo {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 light_pos;
    };
}

struct objects {
    ex::object dragon;
    ex::object monkey;
    ex::object grid;
} objects;

struct meshes {
    ex::mesh dragon;
    ex::mesh monkey;
    ex::mesh grid;
} _meshes;

struct vulkan_models {
    ex::vulkan::model dragon;
    ex::vulkan::model monkey;
    ex::vulkan::model grid;
} _models;

struct vulkan_shaders {
    ex::vulkan::shader colored;
    ex::vulkan::shader textured;
} _shaders;

struct vulkan_descriptor_sets {
    ex::vulkan::descriptor_set uniform;
    ex::vulkan::descriptor_set textures;
} _descriptor_sets;

struct vulkan_pipelines {
    ex::vulkan::pipeline colored;
    ex::vulkan::pipeline textured;
} _pipelines;

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

    _meshes.dragon.load_file("res/meshes/dragon.obj");
    _meshes.monkey.load_file("res/meshes/monkey_smooth.obj");
    std::vector<ex::vertex> vertices;
    std::vector<uint32_t> indices;
    create_grid(10, &vertices, &indices);
    _meshes.grid.load_array(vertices, indices);

    _models.dragon.create(&backend, &_meshes.dragon);
    _models.monkey.create(&backend, &_meshes.monkey);
    _models.grid.create(&backend, &_meshes.grid);

    EXDEBUG("Dragon mesh vertex count: %d", _models.dragon.vertex_count());
    EXDEBUG("Dragon mesh index count: %d", _models.dragon.index_count());
    objects.dragon.transform.translation = glm::vec3(-1.5f, 0.0f, 0.0f);
    objects.dragon.transform.rotation = glm::vec3(0.0f, -60.0f, 0.0f);
    objects.dragon.transform.scale = glm::vec3(0.25f);

    EXDEBUG("Monkey mesh vertex count: %d", _models.monkey.vertex_count());
    EXDEBUG("Monkey mesh index count: %d", _models.monkey.index_count());
    objects.monkey.transform.translation = glm::vec3(1.5f, 1.0f, 0.0f);
    objects.monkey.transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    objects.monkey.transform.scale = glm::vec3(1.0f);

    EXDEBUG("Grid mesh vertex count: %d", _models.grid.vertex_count());
    EXDEBUG("Grid mesh index count: %d", _models.grid.index_count());
    objects.grid.transform.translation = glm::vec3(-5.0f, 0.0f, -5.0f);
    objects.grid.transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    objects.grid.transform.scale = glm::vec3(1.0f);
        
    ex::vulkan::texture texture;
    texture.create(&backend, "res/textures/paris.jpg");
    
    ex::vulkan::buffer uniform_buffer;
    uniform_buffer.set_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    uniform_buffer.set_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    uniform_buffer.build(&backend, sizeof(ex::ubo));
    uniform_buffer.bind(&backend);
    
    ex::vulkan::descriptor_pool descriptor_pool;
    descriptor_pool.add_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
    descriptor_pool.add_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
    descriptor_pool.create(&backend, 2);
    
    _descriptor_sets.uniform.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
    _descriptor_sets.uniform.build_layout(&backend);
    _descriptor_sets.uniform.allocate(&backend, &descriptor_pool);
    _descriptor_sets.uniform.add_write_set(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, uniform_buffer.get_descriptor_info());
    _descriptor_sets.uniform.update(&backend);

    _descriptor_sets.textures.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
    _descriptor_sets.textures.build_layout(&backend);
    _descriptor_sets.textures.allocate(&backend, &descriptor_pool);
    _descriptor_sets.textures.add_write_set(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texture.get_descriptor_info(), nullptr);
    _descriptor_sets.textures.update(&backend);

    _shaders.colored.create(&backend, "res/shaders/default.vert.spv", "res/shaders/colored.frag.spv");
    _shaders.textured.create(&backend, "res/shaders/default.vert.spv", "res/shaders/textured.frag.spv");

    _pipelines.colored.push_descriptor_set_layout(_descriptor_sets.uniform.layout());
    _pipelines.colored.set_push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ex::push_constants));
    _pipelines.colored.build_layout(&backend);
    _pipelines.colored.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.colored.set_polygon_mode(VK_POLYGON_MODE_FILL);
    _pipelines.colored.set_cull_mode(VK_CULL_MODE_BACK_BIT);
    _pipelines.colored.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    _pipelines.colored.build(&backend, &_shaders.colored);

    _pipelines.textured.push_descriptor_set_layout(_descriptor_sets.uniform.layout());
    _pipelines.textured.push_descriptor_set_layout(_descriptor_sets.textures.layout());
    _pipelines.textured.set_push_constant_range(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ex::push_constants));
    _pipelines.textured.build_layout(&backend);
    _pipelines.textured.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.textured.set_polygon_mode(VK_POLYGON_MODE_FILL);
    _pipelines.textured.set_cull_mode(VK_CULL_MODE_BACK_BIT);
    _pipelines.textured.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    _pipelines.textured.build(&backend, &_shaders.textured);

    _shaders.colored.destroy(&backend);
    _shaders.textured.destroy(&backend);

    EXINFO("-=+INITIALIZED+=-");
    // NOTE: camera orientation is inverted
    ex::camera camera = {};
    camera.set_position({0.0f, 2.0f, 4.0f});
    camera.set_target({0.0f, 0.0f, 1.0f});
    camera.set_up({0.0f, -1.0f, 0.0f});
    camera.set_perspective(80.0f, (float) window.width() / (float) window.height(), 0.1f, 20.0f);
    camera.set_speed(2.5f);
    camera.set_sens(100.0f);

    float light_speed = 0;
    
    using clock = std::chrono::high_resolution_clock;
    auto last_time = clock::now();
        
    while (window.is_active()) {        
        window.update();
        
        auto now = clock::now();
        auto delta = std::chrono::duration<float, std::chrono::seconds::period>(now - last_time).count();
        last_time = now;

        if (input.key_pressed(EX_KEY_ESCAPE)) window.close();
        if (input.key_pressed(EX_KEY_F1)) window.change_display_mode();
        
        if (input.button_pressed(EX_BUTTON_LEFT)) EXDEBUG("MOUSE LEFT BUTTON PRESSED");
        if (input.button_pressed(EX_BUTTON_RIGHT)) EXDEBUG("MOUSE RIGHT BUTTON PRESSED");
        if (input.button_pressed(EX_BUTTON_MIDDLE)) EXDEBUG("MOUSE MIDDLE BUTTON PRESSED");
        
        camera.update(&input, delta);
        camera.update_aspect_ratio(backend.get_swapchain_aspect_ratio());
        
        ex::ubo ubo = {};
        ubo.view = camera.get_view();
        ubo.projection = camera.get_projection();
        
        light_speed += 100.0f * delta;
        ubo.light_pos = glm::vec3(0.0f, 3.0f, 6.0f);
        ubo.light_pos = glm::rotate(ubo.light_pos, glm::radians(light_speed) , glm::vec3(0.0f, 1.0f, 0.0f));

        uniform_buffer.map(&backend);
        uniform_buffer.copy_to(&ubo, sizeof(ex::ubo));
        uniform_buffer.unmap(&backend);

        float rotation_speed = 50.0f * delta;
        objects.dragon.transform.rotation.y += rotation_speed;
        objects.monkey.transform.rotation.y += rotation_speed;
        //objects.grid.transform.rotation.y += rotation_speed;

        float speed = 2.5f * delta;
        if (input.key_down(EX_KEY_LEFT)) objects.monkey.transform.translation -= glm::vec3(speed, 0.0f, 0.0f);
        if (input.key_down(EX_KEY_RIGHT)) objects.monkey.transform.translation += glm::vec3(speed, 0.0f, 0.0f);
        if (input.key_down(EX_KEY_UP)) objects.monkey.transform.translation += glm::vec3(0.0f, 0.0f, speed);
        if (input.key_down(EX_KEY_DOWN)) objects.monkey.transform.translation -= glm::vec3(0.0f, 0.0f, speed);
        
        backend.begin_render();
        if (!window.is_minimized()) {
            std::vector<VkDescriptorSet> sets = {_descriptor_sets.uniform.handle()};            
            _pipelines.colored.bind(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            _pipelines.colored.update_dynamic(backend.current_frame(), backend.swapchain_extent());
            _pipelines.colored.bind_descriptor_sets(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
            
            ex::push_constants constants = {};
            constants.transform = objects.monkey.transform.matrix();
            _pipelines.colored.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT, &constants);
            _models.monkey.bind(backend.current_frame());
            _models.monkey.draw(backend.current_frame());
            
            constants.transform = objects.grid.transform.matrix();
            _pipelines.colored.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT, &constants);
            _models.grid.bind(backend.current_frame());
            _models.grid.draw(backend.current_frame());

            // sets.push_back(_descriptor_sets.textures.handle());
            // _pipelines.textured.bind(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            // _pipelines.textured.update_dynamic(backend.current_frame(), backend.swapchain_extent());
            // _pipelines.textured.bind_descriptor_sets(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
            
            constants.transform = objects.dragon.transform.matrix();
            //_pipelines.textured.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT, &constants);
            _pipelines.colored.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT, &constants);
            _models.dragon.bind(backend.current_frame());
            _models.dragon.draw(backend.current_frame());
        }
        
        backend.end_render();
        input.update();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //window.sleep(1);
    }
    
    EXINFO("-=+SHUTTING_DOWN+=-");
    _pipelines.textured.destroy(&backend);
    _pipelines.colored.destroy(&backend);
    
    _descriptor_sets.textures.destroy_layout(&backend);
    _descriptor_sets.uniform.destroy_layout(&backend);
    descriptor_pool.destroy(&backend);

    uniform_buffer.destroy(&backend);
    texture.destroy(&backend);

    _models.grid.destroy(&backend);
    _models.monkey.destroy(&backend);
    _models.dragon.destroy(&backend);
    
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
