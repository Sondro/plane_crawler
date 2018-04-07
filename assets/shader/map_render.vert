#version 330 core

in vec3 in_position;
in vec2 in_uv;
in vec3 in_normal;

out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    uv = in_uv;
    gl_Position = projection * view * model * vec4(in_position, 1.0);
}
