#version 330 core

in vec3 in_position;
in vec3 in_normal;
in vec3 in_color;

out vec3 vert_normal;
out vec3 vert_color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vert_normal = in_normal;
    vert_color = in_color;
    gl_Position = projection * view * model * vec4(in_position, 1.0);
}
