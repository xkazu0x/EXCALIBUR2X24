#version 450

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_camera_pos;
layout (location = 4) in vec3 in_light_pos;

layout (location = 0) out vec4 out_frag_color;

layout (push_constant) uniform Push {
    mat4 model;
    vec4 color;
} push;

void main() {
    //out_frag_color = push.color;

    vec3 normal = normalize(in_normal);
    vec3 light = normalize(in_light_pos);
    vec3 camera = normalize(in_camera_pos);
    vec3 reflection = reflect(light, normal);

    // PHONG
    // vec3 ambient_light = vec3(push.color) * 0.1;
    // vec3 diffuse_light = max(dot(normal, light), 0.0) * vec3(push.color);
    // vec3 specular_light = pow(max(dot(reflection, camera), 0.0), 16.0) * vec3(1.35);
    // out_frag_color = vec4(ambient_light + diffuse_light + specular_light, 1.0);

    // CARTOON
    if (pow(max(dot(reflection, camera), 0.0), 5.0) > 0.5) {
        out_frag_color = vec4(vec3(push.color), 1.0);
    } else if (dot(-camera, normal) < 0.5) {
        //out_frag_color = vec4(vec3(push.color) * 0.1, 1.0);
    } else if (max(dot(normal, light), 0.0) >= 0.1) {
        //out_frag_color = vec4(vec3(push.color) * 0.5, 1.0);
    } else {
        //out_frag_color = vec4(vec3(push.color) * 0.3, 1.0);
    }
}
