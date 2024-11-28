#version 450

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 out_frag_color;

layout (binding = 1) uniform sampler2D sampler_texture;

void main() {
    //out_frag_color = vec4(in_color, 1.0);
    out_frag_color = texture(sampler_texture, in_uv);
}
