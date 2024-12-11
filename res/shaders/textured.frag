#version 450

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_camera_pos;
layout (location = 4) in vec3 in_light_pos;

layout (location = 0) out vec4 out_frag_color;

layout (binding = 1) uniform sampler2D sampler_texture;

void main() {
    //out_frag_color = vec4(in_color, 1.0);
    //out_frag_color = texture(sampler_texture, in_uv);
    
    vec3 normal = normalize(in_normal);
    vec3 light = normalize(in_light_pos);
    vec3 camera = normalize(in_camera_pos);
    vec3 reflection = reflect(light, normal);

    // TEXTURED
    vec3 texture_map = vec3(texture(sampler_texture, in_uv));
    vec3 ambient_light = 0.1 * texture_map;
    vec3 diffuse_light = max(dot(normal, light), 0.0) * texture_map;
    out_frag_color = vec4(ambient_light + diffuse_light, 1.0);

    // PHONG
    // vec3 ambient_light = in_color * 0.1;
    // vec3 diffuse_light = max(dot(normal, light), 0.0) * in_color;
    // vec3 specular_light = pow(max(dot(reflection, camera), 0.0), 16.0) * vec3(1.35);
    // out_frag_color = vec4(ambient_light + diffuse_light + specular_light, 1.0);

    // NORMAL MAP
    //out_frag_color = vec4(normal, 1.0);
}
