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

#include <cmath>
#include <memory>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

ex::input input;
ex::platform::window window;
ex::platform::timer timer;
ex::vulkan::backend backend;

namespace ex {
    struct aabb {
        glm::vec3 min;
        glm::vec3 max;
        ex::mesh mesh;
        ex::vulkan::model model;
    };

    struct entity {
        ex::vulkan::model model;
        ex::vulkan::model aabb_model;
        ex::comp::transform transform;
        ex::aabb aabb;
    };
}

struct meshes {
    ex::mesh monkey;
    ex::mesh grid;
} _meshes;

struct aabbs {
    ex::aabb monkey;
    ex::aabb grid;
} _aabbs;

struct vulkan_models {
    ex::vulkan::model monkey;
    ex::vulkan::model grid;
} _models;

struct vulkan_textures {
    ex::vulkan::texture goreshit;
    ex::vulkan::texture paris;
} _textures;

struct vulkan_shaders {
    ex::vulkan::shader solid_color;
    ex::vulkan::shader textured;
} _shaders;

struct vulkan_pipelines {
    ex::vulkan::pipeline solid_color;
    ex::vulkan::pipeline textured;
} _pipelines;

struct vulkan_descriptor_sets {
    ex::vulkan::descriptor_set uniform;
    ex::vulkan::descriptor_set textures;
} _descriptor_sets;

struct render_data {
    std::vector<ex::vulkan::model*> models;
    std::vector<ex::vulkan::model*> aabb_models;
    std::vector<ex::comp::transform*> transforms;
} _render_data;

struct push_constants {
    glm::mat4 model;
    glm::vec4 color;
};

struct ubo {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 light_pos;
};

void create_grid(uint32_t size, std::vector<ex::vertex> *vertices, std::vector<uint32_t> *indices);
ex::aabb create_aabb(std::vector<ex::vertex> vertices);

void
mesh_render_pass(ex::vulkan::backend *backend,
                 ex::vulkan::pipeline *pipeline,
                 std::vector<VkDescriptorSet> &descriptor_sets,
                 render_data *render_data,
                 glm::vec3 color) {
    pipeline->bind(backend->current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
    pipeline->update_dynamic(backend->current_frame(), backend->swapchain_extent());
    pipeline->bind_descriptor_sets(backend->current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, descriptor_sets);
    
    push_constants constants = {};
    constants.color = glm::vec4(color, 1.0f);

    for (uint32_t i = 0; i < render_data->models.size(); i++) {
        constants.model = render_data->transforms[i]->matrix();
        pipeline->push_constants(backend->current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
        
        render_data->models[i]->bind(backend->current_frame());
        render_data->models[i]->draw(backend->current_frame());
    }
}

void
aabb_render_pass(ex::vulkan::backend *backend,
                 ex::vulkan::pipeline *pipeline,
                 std::vector<VkDescriptorSet> &descriptor_sets,
                 render_data *render_data,
                 glm::vec3 color) {
    pipeline->bind(backend->current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
    pipeline->update_dynamic(backend->current_frame(), backend->swapchain_extent());
    pipeline->bind_descriptor_sets(backend->current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, descriptor_sets);
    
    push_constants constants = {};
    constants.color = glm::vec4(color, 1.0f);

    for (uint32_t i = 0; i < render_data->aabb_models.size(); i++) {
        constants.model = render_data->transforms[i]->matrix();
        pipeline->push_constants(backend->current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
        
        render_data->aabb_models[i]->bind(backend->current_frame());
        render_data->aabb_models[i]->draw(backend->current_frame());
    }
}

bool
intersect(ex::entity *entity, ex::entity *other) {
    float left = entity->transform.translation.x - std::abs(entity->aabb.min.x * entity->transform.scale.x);
    float right = entity->transform.translation.x + std::abs(entity->aabb.max.x * entity->transform.scale.x);
    float back = entity->transform.translation.z - std::abs(entity->aabb.min.z * entity->transform.scale.z);
    float front = entity->transform.translation.z + std::abs(entity->aabb.max.z * entity->transform.scale.z);
    float bottom = entity->transform.translation.y - std::abs(entity->aabb.min.y * entity->transform.scale.y);
    float top = entity->transform.translation.y + std::abs(entity->aabb.max.y * entity->transform.scale.y);
    
    float oleft = other->transform.translation.x - std::abs(other->aabb.min.x * other->transform.scale.x);
    float oright = other->transform.translation.x + std::abs(other->aabb.max.x * other->transform.scale.x);
    float oback = other->transform.translation.z - std::abs(other->aabb.min.z * other->transform.scale.z);
    float ofront = other->transform.translation.z + std::abs(other->aabb.max.z * other->transform.scale.z);
    float obottom = other->transform.translation.y - std::abs(other->aabb.min.y * other->transform.scale.y);
    float otop = other->transform.translation.y + std::abs(other->aabb.max.y * other->transform.scale.y);

    return !(left >= oright || right <= oleft ||
             back >= ofront || front <= oback ||
             bottom >= otop || top <= obottom);
}

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

    _textures.goreshit.create(&backend, "res/textures/goreshit.jpg");
    _textures.paris.create(&backend, "res/textures/parisx.jpg");
        
    // create resources
    _meshes.monkey.load_file("res/meshes/monkey_smooth.obj");
    _models.monkey.create(&backend, &_meshes.monkey);
    
    _aabbs.monkey = create_aabb(_meshes.monkey.vertices());
    _aabbs.monkey.model.create(&backend, &_aabbs.monkey.mesh);

    std::vector<ex::vertex> vertices;
    std::vector<uint32_t> indices;
    create_grid(10, &vertices, &indices);
    _meshes.grid.load_array(vertices, indices);
    _models.grid.create(&backend, &_meshes.grid);
    
    _aabbs.grid = create_aabb(_meshes.grid.vertices());    
    _aabbs.grid.model.create(&backend, &_aabbs.grid.mesh);

    // entities
    ex::entity monkey;
    monkey.model = _models.monkey;
    monkey.aabb = _aabbs.monkey;
    
    ex::entity monkey1;
    monkey1.model = _models.monkey;
    monkey1.aabb = _aabbs.monkey;

    ex::entity monkey2;
    monkey2.model = _models.monkey;
    monkey2.aabb = _aabbs.monkey;

    ex::entity grid;
    grid.model = _models.grid;
    grid.aabb = _aabbs.grid;
    
    // initialize transforms
    monkey.transform.translation = glm::vec3(0.0f, 2.0f, 0.0f);
    monkey.transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    monkey.transform.scale = glm::vec3(0.8f);
    
    monkey1.transform.translation = glm::vec3(0.0f, 4.0f, 0.0f);
    monkey1.transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    monkey1.transform.scale = glm::vec3(0.8f);
    
    monkey2.transform.translation = glm::vec3(0.0f, 6.0f, 0.0f);
    monkey2.transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    monkey2.transform.scale = glm::vec3(0.8f);

    grid.transform.translation = glm::vec3(0.0f);
    grid.transform.rotation = glm::vec3(0.0f);
    grid.transform.scale = glm::vec3(1.0f);
        
    // push to render data struct
    _render_data.models.push_back(&monkey.model);
    _render_data.models.push_back(&monkey1.model);
    _render_data.models.push_back(&monkey2.model);
    _render_data.models.push_back(&grid.model);
    _render_data.aabb_models.push_back(&monkey.aabb.model);
    _render_data.aabb_models.push_back(&monkey1.aabb.model);
    _render_data.aabb_models.push_back(&monkey2.aabb.model);
    _render_data.aabb_models.push_back(&grid.aabb.model);
    _render_data.transforms.push_back(&monkey.transform);
    _render_data.transforms.push_back(&monkey1.transform);
    _render_data.transforms.push_back(&monkey2.transform);
    _render_data.transforms.push_back(&grid.transform);
    
    ex::vulkan::buffer uniform_buffer;
    uniform_buffer.set_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    uniform_buffer.set_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    uniform_buffer.build(&backend, sizeof(ubo));
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
    _descriptor_sets.textures.add_write_set(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _textures.goreshit.get_descriptor_info(), nullptr);
    _descriptor_sets.textures.update(&backend);
    
    _shaders.solid_color.create(&backend, "res/shaders/solid_color.vert.spv", "res/shaders/solid_color.frag.spv");
    _shaders.textured.create(&backend, "res/shaders/textured.vert.spv", "res/shaders/textured.frag.spv");
    
    _pipelines.solid_color.push_descriptor_set_layout(_descriptor_sets.uniform.layout());
    _pipelines.solid_color.set_push_constant_range((VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), 0, sizeof(push_constants));
    _pipelines.solid_color.build_layout(&backend);
    _pipelines.solid_color.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.solid_color.set_polygon_mode(VK_POLYGON_MODE_LINE);
    _pipelines.solid_color.set_cull_mode(VK_CULL_MODE_NONE);
    _pipelines.solid_color.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    _pipelines.solid_color.build(&backend, &_shaders.solid_color);

    _pipelines.textured.push_descriptor_set_layout(_descriptor_sets.uniform.layout());
    _pipelines.textured.push_descriptor_set_layout(_descriptor_sets.textures.layout());
    _pipelines.textured.set_push_constant_range((VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), 0, sizeof(push_constants));
    _pipelines.textured.build_layout(&backend);
    _pipelines.textured.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.textured.set_polygon_mode(VK_POLYGON_MODE_FILL);
    _pipelines.textured.set_cull_mode(VK_CULL_MODE_BACK_BIT);
    _pipelines.textured.set_front_face(VK_FRONT_FACE_CLOCKWISE);
    _pipelines.textured.build(&backend, &_shaders.textured);

    _shaders.solid_color.destroy(&backend);
    _shaders.textured.destroy(&backend);

    EXINFO("-=+INITIALIZED+=-");
    ex::camera camera = {};
    camera.set_speed(5.0f);
    camera.set_sens(33.0f);
    camera.set_position({0.0f, 2.0f, 4.0f});
    camera.set_target({0.0f, 0.0f, 1.0f});
    camera.set_up({0.0f, -1.0f, 0.0f});
    camera.set_perspective(80.0f, (float) window.width() / (float) window.height(), 0.1f, 20.0f);

    float light_speed = 0;
    
    bool render_fill = true;
    bool render_line = false;
    bool render_aabb = false;
    
    using clock = std::chrono::high_resolution_clock;

    float fps = 0.0f;
    auto last_time = clock::now();
    float fps_time_counter = 0.0f;
    //float update_timer = 1.0;
    //float frame_time = 1.0f / 60.0f;
    
    while (!window.closed()) {
        auto now = clock::now();
        float delta = std::chrono::duration<float, std::chrono::seconds::period>(now - last_time).count();
        last_time = now;

        fps_time_counter += delta;
        if (fps_time_counter >= 1.0f) {
            float ms_per_frame = 1000.0f / fps;

            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << ms_per_frame << "ms "
               << std::fixed << std::setprecision(2) << fps << "fps";
            std::string title = "EXCALIBUR | " + ss.str();
            window.change_title(title);

            fps_time_counter = 0.0f;
            fps = 0.0f;
        }
        
        window.update();
                        
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

        monkey.transform.rotation.y += -100.0f * delta;
        monkey1.transform.rotation.y += 100.0f * delta;
        monkey2.transform.rotation.y += -100.0f * delta;
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
            std::vector<VkDescriptorSet> sets = {
                _descriptor_sets.uniform.handle(),
                _descriptor_sets.textures.handle(),
            };
            
            if (render_fill) mesh_render_pass(&backend, &_pipelines.textured, sets, &_render_data, glm::vec3(1.0f));

            sets.pop_back();
            if (render_line) mesh_render_pass(&backend, &_pipelines.solid_color, sets, &_render_data, glm::vec3(1.0f, 0.0f, 1.0f));
            if (render_aabb) aabb_render_pass(&backend, &_pipelines.solid_color, sets, &_render_data, glm::vec3(1.0f, 1.0f, 0.0f));
        }
        backend.end_render();
        input.update();

        fps++;
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //window.sleep(1);
    }
    
    EXINFO("-=+SHUTTING_DOWN+=-");
    _pipelines.textured.destroy(&backend);
    _pipelines.solid_color.destroy(&backend);
                        
    _descriptor_sets.textures.destroy_layout(&backend);
    _descriptor_sets.uniform.destroy_layout(&backend);
    descriptor_pool.destroy(&backend);

    uniform_buffer.destroy(&backend);

    _aabbs.grid.model.destroy(&backend);
    _aabbs.monkey.model.destroy(&backend);
    
    _models.grid.destroy(&backend);
    _models.monkey.destroy(&backend);

    _textures.paris.destroy(&backend);
    _textures.goreshit.destroy(&backend);
    
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
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 1.0f + z}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 0.0f + z}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 0.0f + z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f},  {0.0f, 0.0f, 0.0f}));
            
            vertices->push_back(ex::vertex({1.0f + x, 0.0f, 1.0f + z}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 0.0f + z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}));
            vertices->push_back(ex::vertex({0.0f + x, 0.0f, 1.0f + z}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

            for (uint32_t i = 0; i < vertices->size(); i++) {
                indices->push_back(indices->size());
            }
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

    std::vector<ex::vertex> aabb_vertices;
    std::vector<uint32_t> aabb_indices;

    // FRONT
    aabb_vertices.push_back(ex::vertex({min.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    aabb_vertices.push_back(ex::vertex({min.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
                                
    // BACK
    aabb_vertices.push_back(ex::vertex({max.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    aabb_vertices.push_back(ex::vertex({max.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    // LEFT
    aabb_vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    aabb_vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    // RIGHT
    aabb_vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    aabb_vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    // TOP
    aabb_vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    aabb_vertices.push_back(ex::vertex({min.x, max.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, max.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    // BOTTOM
    aabb_vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    aabb_vertices.push_back(ex::vertex({min.x, min.y, max.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({min.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    aabb_vertices.push_back(ex::vertex({max.x, min.y, min.z}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    
    for (uint32_t i = 0; i < aabb_vertices.size(); i++) {
        aabb_indices.push_back(aabb_indices.size());
    }

    out_aabb.mesh.load_array(aabb_vertices, aabb_indices);
    
    return out_aabb;
}
