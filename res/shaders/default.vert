#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in vec3 in_normal;

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out vec3 out_normal;
layout (location = 3) out vec3 out_camera_pos;
layout (location = 4) out vec3 out_light_pos;

layout (binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 light_pos;
} ubo;

void main() {
    vec4 world_pos = ubo.model * vec4(in_position, 1.0);        
    gl_Position = ubo.projection * ubo.view * world_pos;
    
    out_color = in_color;
    out_uv = in_uv;
    out_normal = mat3(ubo.view) * mat3(ubo.model) * in_normal;
    out_camera_pos = (ubo.view * world_pos).xyz;
    out_light_pos = ubo.light_pos - vec3(world_pos);
}
