#include "ex_logger.h"
#include "ex_input.h"
#include "ex_platform.h"

#include "ex_memory.h"

#include "ex_camera.h"
#include "ex_mesh.h"

#include "ex_component.hpp"
#include "ex_entity.hpp"

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

// TODO: custom memory allocator
// TODO: asset manager
// TODO: renderer class
// TODO: game object/entity system

static ex::input _input;
static ex::platform::window _window;
static ex::platform::timer _timer;
static ex::vulkan::backend _backend;

struct engine_stats {
    float delta_time;
    float frame_time;
    uint32_t frames_per_second;
} _stats;

struct meshes {
    ex::mesh floor;    
    ex::mesh monkey;
} _meshes;

struct vulkan_models {
    ex::vulkan::model floor;
    ex::vulkan::model monkey;
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

namespace vulkan {
    struct push_constants {
        glm::mat4 model;
        glm::vec4 color;
    };

    struct ubo {
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 light_pos;
    };
}

// bool
// intersect(ex::entity *entity, ex::entity *other) {
//     float left = entity->transform.translation.x - std::abs(entity->aabb.min.x * entity->transform.scale.x);
//     float right = entity->transform.translation.x + std::abs(entity->aabb.max.x * entity->transform.scale.x);
//     float back = entity->transform.translation.z - std::abs(entity->aabb.min.z * entity->transform.scale.z);
//     float front = entity->transform.translation.z + std::abs(entity->aabb.max.z * entity->transform.scale.z);
//     float bottom = entity->transform.translation.y - std::abs(entity->aabb.min.y * entity->transform.scale.y);
//     float top = entity->transform.translation.y + std::abs(entity->aabb.max.y * entity->transform.scale.y);
    
//     float oleft = other->transform.translation.x - std::abs(other->aabb.min.x * other->transform.scale.x);
//     float oright = other->transform.translation.x + std::abs(other->aabb.max.x * other->transform.scale.x);
//     float oback = other->transform.translation.z - std::abs(other->aabb.min.z * other->transform.scale.z);
//     float ofront = other->transform.translation.z + std::abs(other->aabb.max.z * other->transform.scale.z);
//     float obottom = other->transform.translation.y - std::abs(other->aabb.min.y * other->transform.scale.y);
//     float otop = other->transform.translation.y + std::abs(other->aabb.max.y * other->transform.scale.y);

//     return !(left >= oright || right <= oleft ||
//              back >= ofront || front <= oback ||
//              bottom >= otop || top <= obottom);
// }

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    _input.initialize();

    ex::platform::window::create_info window_create_info = {};
    window_create_info.title = "EXCALIBUR";
    window_create_info.width = 1920 * 0.7;
    window_create_info.height = 1080 * 0.7;
    window_create_info.mode = EX_WINDOW_MODE_WINDOWED;
    window_create_info.pinput = &_input;
    _window.create(&window_create_info);
    _window.show();
    
    if (!_backend.initialize(&_window)) {
        EXFATAL("Failed to initialize vulkan backend");
        return -1;
    }
        
    // load assets
    _meshes.floor.load_file("res/meshes/floor.obj");
    _meshes.monkey.load_file("res/meshes/monkey_smooth.obj");
    
    _models.floor.create(&_backend, &_meshes.floor);
    _models.monkey.create(&_backend, &_meshes.monkey);

    _textures.goreshit.create(&_backend, "res/textures/goreshit.jpg");
    _textures.paris.create(&_backend, "res/textures/parisx.jpg");
    
    // create resources
    ex::vulkan::buffer uniform_buffer;
    uniform_buffer.set_usage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    uniform_buffer.set_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    uniform_buffer.build(&_backend, sizeof(vulkan::ubo));
    uniform_buffer.bind(&_backend);

    // create descriptors
    ex::vulkan::descriptor_pool descriptor_pool;
    descriptor_pool.add_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);
    descriptor_pool.add_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
    descriptor_pool.create(&_backend, 3);

    ex::vulkan::descriptor_set_layout uniform_buffer_layout;
    uniform_buffer_layout.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    uniform_buffer_layout.create(&_backend);

    ex::vulkan::descriptor_set_layout texture_layout;
    texture_layout.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    texture_layout.create(&_backend);
    
    _descriptor_sets.uniform.allocate(&_backend, &descriptor_pool, &uniform_buffer_layout);
    _descriptor_sets.uniform.write_buffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniform_buffer.get_descriptor_info());
    _descriptor_sets.uniform.update(&_backend);
    
    _descriptor_sets.textures.allocate(&_backend, &descriptor_pool, &texture_layout);
    _descriptor_sets.textures.write_image(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _textures.goreshit.get_descriptor_info());
    _descriptor_sets.textures.update(&_backend);

    // create pipelines
    _pipelines.solid_color.push_descriptor_set_layout(uniform_buffer_layout.handle());
    _pipelines.solid_color.set_push_constant_range((VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), 0, sizeof(vulkan::push_constants));
    _pipelines.solid_color.build_layout(&_backend);
    _pipelines.solid_color.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.solid_color.set_polygon_mode(VK_POLYGON_MODE_LINE);
    _pipelines.solid_color.set_cull_mode(VK_CULL_MODE_NONE);
    _pipelines.solid_color.set_front_face(VK_FRONT_FACE_CLOCKWISE);

    _shaders.solid_color.create(&_backend, "res/shaders/solid_color.vert.spv", "res/shaders/solid_color.frag.spv");
    _pipelines.solid_color.build(&_backend, &_shaders.solid_color);
    _shaders.solid_color.destroy(&_backend);

    _pipelines.textured.push_descriptor_set_layout(uniform_buffer_layout.handle());
    _pipelines.textured.push_descriptor_set_layout(texture_layout.handle());
    _pipelines.textured.set_push_constant_range((VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), 0, sizeof(vulkan::push_constants));
    _pipelines.textured.build_layout(&_backend);
    _pipelines.textured.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    _pipelines.textured.set_polygon_mode(VK_POLYGON_MODE_FILL);
    _pipelines.textured.set_cull_mode(VK_CULL_MODE_BACK_BIT);
    _pipelines.textured.set_front_face(VK_FRONT_FACE_CLOCKWISE);

    _shaders.textured.create(&_backend, "res/shaders/textured.vert.spv", "res/shaders/textured.frag.spv");
    _pipelines.textured.build(&_backend, &_shaders.textured);
    _shaders.textured.destroy(&_backend);

    EXINFO("-=+INITIALIZED+=-");
    ex::entity floor;
    floor.model = &_models.floor;
    floor.transform.translation = glm::vec3(0.0f);
    floor.transform.rotation = glm::vec3(0.0f);
    floor.transform.scale = glm::vec3(4.0f);
    
    ex::entity monkey;
    monkey.model = &_models.monkey;
    monkey.transform.translation = glm::vec3(0.0f, 2.0f, 0.0f);
    monkey.transform.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    monkey.transform.scale = glm::vec3(0.8f);

    // TODO: refactor this shitty as camera
    // NOTE: camera is flipped
    ex::camera camera = {};
    camera.set_speed(5.0f);
    camera.set_sens(33.0f);
    camera.set_position(glm::vec3(0.0f, 2.0f, 4.0f));
    camera.set_target({0.0f, 0.0f, 1.0f});
    camera.set_up({0.0f, -1.0f, 0.0f});
    camera.set_perspective(90.0f, (float) _window.width() / (float) _window.height(), 0.1f, 20.0f);

    bool render_fill = true;
    bool render_line = false;

    _timer.init();
    uint64_t last_time = _timer.get_time();
    float time_counter = 0.0f;

    while (!_window.closed()) {
        _window.update();
        
        uint64_t start = _timer.get_time();
        
        if (_input.key_pressed(EX_KEY_ESCAPE)) _window.close();
        if (_input.key_pressed(EX_KEY_F1)) _window.toggle_fullscreen();

        if (_input.key_pressed(EX_KEY_1)) render_fill = !render_fill;
        if (_input.key_pressed(EX_KEY_2)) render_line = !render_line;

        camera.update_matrix(_backend.swapchain_extent().width, _backend.swapchain_extent().height);
        camera.update_input(&_input, _stats.delta_time);
        if (!camera.is_locked()) {
            _window.set_cursor_pos(_backend.swapchain_extent().width / 2, _backend.swapchain_extent().height / 2);
            _window.hide_cursor(true);
        } else { _window.hide_cursor(false); }
        
        vulkan::ubo ubo = {};
        ubo.view = camera.get_view();
        ubo.projection = camera.get_projection();
        ubo.light_pos = glm::vec3(0.0f, 4.0f, 0.0f);

        uniform_buffer.map(&_backend);
        uniform_buffer.copy_to(&ubo, sizeof(vulkan::ubo));
        uniform_buffer.unmap(&_backend);

        if (!_window.inactive()) {
            _backend.begin_render();
            
            std::vector<VkDescriptorSet> sets = {
                _descriptor_sets.uniform.handle(),
            };

            if (render_fill) {
                vulkan::push_constants constants = {};
                constants.color = glm::vec4(1.0f);
            
                _pipelines.textured.bind(_backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
                _pipelines.textured.update_dynamic(_backend.current_frame(), _backend.swapchain_extent());
                _pipelines.textured.bind_descriptor_sets(_backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
    
                constants.model = monkey.transform.matrix();
                _pipelines.textured.push_constants(_backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);

                sets.push_back(_descriptor_sets.textures.handle());
                _pipelines.textured.bind_descriptor_sets(_backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
                monkey.model->bind(_backend.current_frame());
                monkey.model->draw(_backend.current_frame());
                sets.pop_back();
        
                constants.model = floor.transform.matrix();
                _pipelines.textured.push_constants(_backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
        
                sets.push_back(_descriptor_sets.textures.handle());
                _pipelines.textured.bind_descriptor_sets(_backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
                floor.model->bind(_backend.current_frame());
                floor.model->draw(_backend.current_frame());
                sets.pop_back();
            }

            if (render_line) {
                vulkan::push_constants constants = {};
                constants.color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
            
                _pipelines.solid_color.bind(_backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS);
                _pipelines.solid_color.update_dynamic(_backend.current_frame(), _backend.swapchain_extent());
                _pipelines.solid_color.bind_descriptor_sets(_backend.current_frame(), VK_PIPELINE_BIND_POINT_GRAPHICS, sets);
    
                constants.model = monkey.transform.matrix();
                _pipelines.solid_color.push_constants(_backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
                monkey.model->bind(_backend.current_frame());
                monkey.model->draw(_backend.current_frame());
        
                constants.model = floor.transform.matrix();
                _pipelines.solid_color.push_constants(_backend.current_frame(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &constants);
                floor.model->bind(_backend.current_frame());
                floor.model->draw(_backend.current_frame());
            }
            
            _backend.end_render();
        }
        
        _input.update();
        
        uint64_t end = _timer.get_time();
        uint32_t elapsed = static_cast<uint32_t>(end - start);
        _stats.delta_time = (float)(end - last_time) / (float)(_timer.get_frequency());
        _stats.frame_time = ((1000.0f * (float)elapsed) / (float)_timer.get_frequency());
        _stats.frames_per_second++;
        last_time = end;
        
        time_counter += _stats.delta_time;
        if (time_counter >= 1.0f) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << _stats.frame_time << "ms "
               << std::fixed << std::setprecision(2) << _stats.frames_per_second << "fps";
            std::string title = "EXCALIBUR | " + ss.str();
            _window.change_title(title);
            
            time_counter = 0.0f;
            _stats.frames_per_second = 0;
        }
        
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        //_window.sleep(1);
    }
    
    EXINFO("-=+SHUTTING_DOWN+=-");
    _backend.wait_idle();
    _pipelines.textured.destroy(&_backend);
    _pipelines.solid_color.destroy(&_backend);
                        
    texture_layout.destroy(&_backend);
    uniform_buffer_layout.destroy(&_backend);
    descriptor_pool.destroy(&_backend);

    uniform_buffer.destroy(&_backend);
    
    _textures.paris.destroy(&_backend);
    _textures.goreshit.destroy(&_backend);
    
    _models.monkey.destroy(&_backend);
    _models.floor.destroy(&_backend);
    _backend.shutdown();
    
    _window.destroy();
    _input.shutdown();
    
    return 0;
}
