#version 330 core

in vec2 uv;
in vec3 vert_normal;

out vec4 color;

uniform vec4 rect_color;
uniform vec2 thickness;

void main() {
    if(thickness.x < 0 ||
       uv.x <= thickness.x || uv.x >= 1-thickness.x ||
       uv.y <= thickness.y || uv.y >= 1-thickness.y) {
        color = rect_color;
    }
    else {
        discard;
    }
}
