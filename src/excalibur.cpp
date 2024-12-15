#include "ex_logger.h"
#include "ex_input.h"
#include "ex_platform.h"

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
ex::platform::window window;
ex::platform::timer timer;
ex::vulkan::backend backend;

namespace push_constants {
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
    struct aabb {
        std::vector<ex::vertex> vertices;
        std::vector<uint32_t> indices;
        glm::vec3 min;
        glm::vec3 max;
    };

    struct entity {
        ex::mesh mesh;
        ex::vulkan::model model;
        ex::aabb aabb;
        ex::mesh aabb_mesh;
        ex::vulkan::model aabb_model;
        ex::comp::transform transform;
    };
}

struct vulkan_shaders {
    ex::vulkan::shader solid_color;
    ex::vulkan::shader memory_bug;
} _shaders;

struct vulkan_pipelines {
    ex::vulkan::pipeline solid_color;
    ex::vulkan::pipeline memory_bug;
} _pipelines;

struct vulkan_descriptor_sets {
    ex::vulkan::descriptor_set uniform;
} _descriptor_sets;

void create_grid(uint32_t size, std::vector<ex::vertex> *vertices, std::vector<uint32_t> *indices);
ex::aabb create_aabb(std::vector<ex::vertex> vertices);

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    input.initialize();

    ex::platform::window::create_info window_create_info = {};
    window_create_info.title = "EXCALIBUR";
    window_create_info.width = 1920 * 0.7;
    window_create_info.height = 1080 * 0.7;
    window_create_info.mode = ex::platform::window::mode::WINDOWED;
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

    monkey.aabb = create_aabb(monkey.mesh.vertices());
    monkey.aabb_mesh.load_array(monkey.aabb.vertices, monkey.aabb.indices);
    monkey.aabb_model.create(&backend, &monkey.aabb_mesh);
    
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
    _shaders.memory_bug.create(&backend, "res/shaders/memory_bug.vert.spv", "res/shaders/memory_bug.frag.spv");
    
    _pipelines.solid_color.push_descriptor_set_layout(_descriptor_sets.uniform.layout());
    _pipelines.solid_color.set_push_constant_range((VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), 0, sizeof(push_constants::color));
    _pipelines.solid_color.build_layout(&backend);
    _pipelines.solid_color.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.solid_color.set_polygon_mode(VK_POLYGON_MODE_LINE);
    _pipelines.solid_color.set_cull_mode(VK_CULL_MODE_NONE);
    _pipelines.solid_color.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    _pipelines.solid_color.build(&backend, &_shaders.solid_color);

    _pipelines.memory_bug.push_descriptor_set_layout(_descriptor_sets.uniform.layout());
    _pipelines.memory_bug.set_push_constant_range((VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), 0, sizeof(push_constants::color));
    _pipelines.memory_bug.build_layout(&backend);
    _pipelines.memory_bug.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.memory_bug.set_polygon_mode(VK_POLYGON_MODE_FILL);
    _pipelines.memory_bug.set_cull_mode(VK_CULL_MODE_BACK_BIT);
    _pipelines.memory_bug.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    _pipelines.memory_bug.build(&backend, &_shaders.memory_bug);

    _shaders.solid_color.destroy(&backend);
    _shaders.memory_bug.destroy(&backend);

    EXINFO("-=+INITIALIZED+=-");
    // NOTE: camera orientation is inverted
    ex::camera camera = {};
    camera.set_speed(5.0f);
    camera.set_sens(33.0f);
    camera.set_position({0.0f, 2.0f, 4.0f});
    camera.set_target({0.0f, 0.0f, 1.0f});
    camera.set_up({0.0f, -1.0f, 0.0f});
    camera.set_perspective(80.0f, (float) window.width() / (float) window.height(), 0.1f, 20.0f);                        
    
    grid.transform.translation = glm::vec3(0.0f, 0.0f, 0.0f);
    grid.transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    grid.transform.scale = glm::vec3(1.0f);

    bool render_fill = true;
    bool render_line = false;
    bool render_aabb = false;
        
    float light_speed = 0;

    // INIT TIMER
    // int monitor_refresh_hz = 60;
    // int game_update_hz = monitor_refresh_hz / 2;
    // float target_seconds_per_frame = 1.0f / (float)game_update_hz;
    
    // timer.init(target_seconds_per_frame);
    // timer.start();
    
    using clock = std::chrono::high_resolution_clock;
    auto last_time = clock::now();
    while (!window.closed()) {
        window.update();
        
        // timer.end();
        // std::stringstream ss;
        // ss << std::fixed << std::setprecision(2) << timer.ms() << "ms "
        //    << std::fixed << std::setprecision(2) << timer.fps() << "fps "
        //    << std::fixed << std::setprecision(2) << timer.mc() << "mc";
        // std::string title = "EXCALIBUR | " + ss.str();
        // window.change_title(title);
        //EXDEBUG("%.02fms, %.02ffps, %.02fmc", ms_per_frame, frames_per_second, mega_cycles_per_frame);
        
        auto now = clock::now();
        float delta = std::chrono::duration<float, std::chrono::seconds::period>(now - last_time).count();
        last_time = now;
        
        if (input.key_pressed(EX_KEY_ESCAPE)) window.close();
        if (input.key_pressed(EX_KEY_F1)) window.toggle_fullscreen();

        if (input.key_pressed(EX_KEY_1)) render_fill = !render_fill;
        if (input.key_pressed(EX_KEY_2)) render_line = !render_line;
        if (input.key_pressed(EX_KEY_3)) render_aabb = !render_aabb;
        
        camera.update_matrix(backend.swapchain_extent().width, backend.swapchain_extent().height);
        camera.update_input(&input, delta);
        if (!camera.is_locked()) {
            window.set_cursor_pos(backend.swapchain_extent().width / 2, backend.swapchain_extent().height / 2);
            window.hide_cursor(true);
        } else { window.hide_cursor(false); }

        monkey.transform.rotation.y += 100.0f * delta;
        monkey.transform.scale = glm::vec3(0.8f);
        
        grid.transform.rotation.y += 100.0f * delta;
        
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

            if (render_fill) {
                _pipelines.memory_bug.bind(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
                _pipelines.memory_bug.update_dynamic(backend.current_frame(), backend.swapchain_extent());
                _pipelines.memory_bug.bind_descriptor_sets(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);

                push_constants::color constants = {};
                constants.color = glm::vec4(1.0f);
                
                for (float y = 0.5f; y < 3.0f; y++) {
                    for (int x = -1; x < 2; x++) {
                        float spread = 3.0f;
                        monkey.transform.translation = glm::vec3(x * spread, y * spread, -4.0f);
                        
                        constants.model = monkey.transform.matrix();
                        _pipelines.memory_bug.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
                        monkey.model.bind(backend.current_frame());
                        monkey.model.draw(backend.current_frame());
                    }
                }
            
                constants.model = grid.transform.matrix();
                _pipelines.memory_bug.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
                grid.model.bind(backend.current_frame());
                grid.model.draw(backend.current_frame());
            }
            
            if (render_line) {
                _pipelines.solid_color.bind(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
                _pipelines.solid_color.update_dynamic(backend.current_frame(), backend.swapchain_extent());
                _pipelines.solid_color.bind_descriptor_sets(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
            
                push_constants::color constants = {};
                constants.color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
                
                for (float y = 0.5f; y < 3.0f; y++) {
                    for (int x = -1; x < 2; x++) {
                        float spread = 3.0f;
                        monkey.transform.translation = glm::vec3(x * spread, y * spread, -4.0f);
                        
                        constants.model = monkey.transform.matrix();
                        _pipelines.solid_color.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
                        monkey.model.bind(backend.current_frame());
                        monkey.model.draw(backend.current_frame());
                    }
                }
            
                constants.model = grid.transform.matrix();
                _pipelines.solid_color.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
                grid.model.bind(backend.current_frame());
                grid.model.draw(backend.current_frame());
            }

            if (render_aabb) {
                _pipelines.solid_color.bind(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
                _pipelines.solid_color.update_dynamic(backend.current_frame(), backend.swapchain_extent());
                _pipelines.solid_color.bind_descriptor_sets(backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
            
                push_constants::color constants = {};
                constants.color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
                
                for (float y = 0.5f; y < 3.0f; y++) {
                    for (int x = -1; x < 2; x++) {
                        float spread = 3.0f;
                        monkey.transform.translation = glm::vec3(x * spread, y * spread, -4.0f);
                        
                        constants.model = monkey.transform.matrix();
                        _pipelines.solid_color.push_constants(backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
                        monkey.aabb_model.bind(backend.current_frame());
                        monkey.aabb_model.draw(backend.current_frame());
                    }
                }
            }
        }
        
        backend.end_render();
        input.update();

        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //window.sleep(1);
    }
    
    EXINFO("-=+SHUTTING_DOWN+=-");
    _pipelines.memory_bug.destroy(&backend);
    _pipelines.solid_color.destroy(&backend);
                        
    _descriptor_sets.uniform.destroy_layout(&backend);
    descriptor_pool.destroy(&backend);

    uniform_buffer.destroy(&backend);

    grid.model.destroy(&backend);
    monkey.aabb_model.destroy(&backend);
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

ex::aabb
create_aabb(std::vector<ex::vertex> vertices) {
    glm::vec3 min = glm::vec3(0.0f);
    glm::vec3 max = glm::vec3(0.0f);
    for (ex::vertex &vertex : vertices) {
        if (vertex.position.x < min.x) min.x = vertex.position.x;
        if (vertex.position.y < min.y) min.y = vertex.position.y;
        if (vertex.position.z < min.z) min.z = vertex.position.z;
        
        if (vertex.position.x > max.x) max.x = vertex.position.x;
        if (vertex.position.y > max.y) max.y = vertex.position.y;
        if (vertex.position.z > max.z) max.z = vertex.position.z;
    }

    ex::aabb out_aabb;
    out_aabb.min = min;
    out_aabb.max = max;

    // FRONT
    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
                                
    // BACK
    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    // LEFT
    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    // RIGHT
    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    // TOP
    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    // BOTTOM
    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({min.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    out_aabb.vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    for (uint32_t i = 0; i < out_aabb.vertices.size(); i++) {
        out_aabb.indices.push_back(out_aabb.indices.size());
    }
    
    return out_aabb;
}
