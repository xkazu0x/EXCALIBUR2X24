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
#include <sstream>
#include <iomanip>

ex::input input;
ex::window window;
ex::vulkan::backend backend;

namespace push_constants {
    struct model {
        glm::mat4 model;
    };

    struct color {
        glm::mat4 model;
        glm::vec4 color;
    };
}

struct ubo {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 light_pos;
};

namespace ex {
    struct entity {
        ex::mesh mesh;
        ex::vulkan::model model;
        ex::comp::transform transform;
    };
}

struct vulkan_shaders {
    ex::vulkan::shader solid_color;
} _shaders;

struct vulkan_pipelines {
    ex::vulkan::pipeline solid_color;
} _pipelines;

struct vulkan_descriptor_sets {
    ex::vulkan::descriptor_set uniform;
} _descriptor_sets;

void create_grid(uint32_t size, std::vector<ex::vertex> *vertices, std::vector<uint32_t> *indices);

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    input.initialize();

    ex::window::create_info window_create_info = {};
    window_create_info.title = "EXCALIBUR";
    window_create_info.width = 1920 * 0.7;
    window_create_info.height = 1080 * 0.7;
    window_create_info.mode = ex::window::mode::WINDOWED;
    window_create_info.pinput = &input;
    
    window.create(&window_create_info);
    window.show();
    
    if (!backend.initialize(&window)) {
        EXFATAL("Failed to initialize vulkan backend");
        return -1;
    }

    // ex::entity monkey = entity_manager.create_entity();
    // asset_manager.add_mesh(monkey.id, "res/meshes/monkey_smooth.obj");
    // renderer.create_model(monkey.id);
    // components.add_transform(monkey.id);
    // components.add_bounding_box(monkey.id);
    
    ex::entity monkey;
    monkey.mesh.load_file("res/meshes/monkey_smooth.obj");
    EXDEBUG("Monkey vertex count: %d", monkey.mesh.vertices().size());
    EXDEBUG("Monkey index count: %d", monkey.mesh.indices().size());
    monkey.model.create(&backend, &monkey.mesh);

    ex::entity grid;
    std::vector<ex::vertex> vertices;
    std::vector<uint32_t> indices;
    create_grid(10, &vertices, &indices);
    grid.mesh.load_array(vertices, indices);
    grid.model.create(&backend, &grid.mesh);
            
    ex::vulkan::buffer uniform_buffer;
    uniform_buffer.set_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    uniform_buffer.set_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    uniform_buffer.build(&backend, sizeof(ubo));
    uniform_buffer.bind(&backend);
    
    ex::vulkan::descriptor_pool descriptor_pool;
    descriptor_pool.add_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
    descriptor_pool.create(&backend, 1);
    
    _descriptor_sets.uniform.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
    _descriptor_sets.uniform.build_layout(&backend);
    _descriptor_sets.uniform.allocate(&backend, &descriptor_pool);
    _descriptor_sets.uniform.add_write_set(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, uniform_buffer.get_descriptor_info());
    _descriptor_sets.uniform.update(&backend);
    
    _shaders.solid_color.create(&backend, "res/shaders/solid_color.vert.spv", "res/shaders/solid_color.frag.spv");

    _pipelines.solid_color.push_descriptor_set_layout(_descriptor_sets.uniform.layout());
    _pipelines.solid_color.set_push_constant_range((VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), 0, sizeof(push_constants::color));
    _pipelines.solid_color.build_layout(&backend);
    _pipelines.solid_color.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.solid_color.set_polygon_mode(VK_POLYGON_MODE_LINE);
    _pipelines.solid_color.set_cull_mode(VK_CULL_MODE_BACK_BIT);
    _pipelines.solid_color.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    _pipelines.solid_color.build(&backend, &_shaders.solid_color);
    
    _shaders.solid_color.destroy(&backend);

    EXINFO("-=+INITIALIZED+=-");
    // NOTE: camera orientation is inverted
    ex::camera camera = {};
    camera.set_speed(2.5f);
    camera.set_sens(33.0f);
    camera.set_position({0.0f, 2.0f, 4.0f});
    camera.set_target({0.0f, 0.0f, 1.0f});
    camera.set_up({0.0f, -1.0f, 0.0f});
    camera.set_perspective(80.0f, (float) window.width() / (float) window.height(), 0.1f, 20.0f);                        
    
    grid.transform.translation = glm::vec3(0.0f, 0.0f, 0.0f);
    grid.transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    grid.transform.scale = glm::vec3(1.0f);
    
    float light_speed = 0;
    
    using clock = std::chrono::high_resolution_clock;
    auto last_time = clock::now();
    while (!window.closed()) {
        window.update();
        
        auto now = clock::now();
        float delta = std::chrono::duration<float, std::chrono::seconds::period>(now - last_time).count();
        last_time = now;

        if (delta >= 0.0f) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << delta * 1000;
            std::string title = "EXCALIBUR | " + oss.str() + " ms";
            window.change_title(title);
        }        
        
        if (input.key_pressed(EX_KEY_ESCAPE)) window.close();
        if (input.key_pressed(EX_KEY_F1)) window.change_display_mode();

        camera.update_matrix(backend.swapchain_extent().width, backend.swapchain_extent().height);
        camera.update_input(&input, delta);
        if (!camera.is_locked()) {
            window.set_cursor_pos(backend.swapchain_extent().width / 2, backend.swapchain_extent().height / 2);
            //window.show_cursor(false);
        }

        float grid_speed = 100.0f * delta;
        grid.transform.rotation.y += grid_speed;
        
        ubo ubo = {};
        ubo.view = camera.get_view();
        ubo.projection = camera.get_projection();
        
        light_speed += 100.0f * delta;
        ubo.light_pos = glm::vec3(0.0f, 3.0f, 6.0f);
        ubo.light_pos = glm::rotate(ubo.light_pos, glm::radians(light_speed) , glm::vec3(0.0f, 1.0f, 0.0f));

        uniform_buffer.map(&backend);
        uniform_buffer.copy_to(&ubo, sizeof(ubo));
        uniform_buffer.unmap(&backend);
        
        backend.begin_render();
        if (!window.inactive()) {
            std::vector<VkDescriptorSet> sets = {_descriptor_sets.uniform.handle()};
            _pipelines.solid_color.bind(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
            _pipelines.solid_color.update_dynamic(backend.current_frame(), backend.swapchain_extent());
            _pipelines.solid_color.bind_descriptor_sets(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
            
            push_constants::color constants = {};            
            for (float y = 0.5f; y < 3.0f; y++) {
                for (int x = -1; x < 2; x++) {
                    float spread = 3.0f;
                    monkey.transform.translation = glm::vec3(x * spread, y * spread, -4.0f);
                    monkey.transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
                    monkey.transform.scale = glm::vec3(0.8f);
                
                    constants.model = monkey.transform.matrix();
                    constants.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    _pipelines.solid_color.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
                    monkey.model.bind(backend.current_frame());
                    monkey.model.draw(backend.current_frame());
                }
            }
            
            constants.model = grid.transform.matrix();
            constants.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            _pipelines.solid_color.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
            grid.model.bind(backend.current_frame());
            grid.model.draw(backend.current_frame());
        }
        
        backend.end_render();
        input.update();
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //window.sleep(1);
    }
    
    EXINFO("-=+SHUTTING_DOWN+=-");
    _pipelines.solid_color.destroy(&backend);
                        
    _descriptor_sets.uniform.destroy_layout(&backend);
    descriptor_pool.destroy(&backend);

    uniform_buffer.destroy(&backend);

    grid.model.destroy(&backend);
    monkey.model.destroy(&backend);
    
    backend.shutdown();
    window.destroy();
    input.shutdown();
    
    return 0;
}

void create_grid(uint32_t size,
                 std::vector<ex::vertex> *vertices,
                 std::vector<uint32_t> *indices) {
    float sizef = static_cast<float>(size);
    float half = sizef / 2;
    for (float z = -half; z < half; z++) {
        for (float x = -half; x < half; x++) {
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 1.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           //{0.0f, 1.0f, 0.0f}));
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 0.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           //{0.0f, 1.0f, 0.0f}));
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 0.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           //{0.0f, 1.0f, 0.0f}));
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 1.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           //{0.0f, 1.0f, 0.0f}));
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 0.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           //{0.0f, 1.0f, 0.0f}));
                                           {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 1.0f + z},
                                           {1.0f, 1.0f, 1.0f},
                                           {0.0f, 0.0f},
                                           //{0.0f, 1.0f, 0.0f}));
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
