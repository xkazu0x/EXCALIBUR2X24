#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_uv;

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_uv;

layout (binding = 0) uniform UBO {
    mat4 mvp;
} ubo;

void main() {
    gl_Position = ubo.mvp * vec4(in_position, 1.0);
    out_color = in_color;
    out_uv = in_uv;
}
