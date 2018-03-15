#version 330 core

in vec2 uv;
in vec3 vert_normal;

out vec4 color;

uniform sampler2D tex;

void main() {
    color = texture(tex, uv);
}
