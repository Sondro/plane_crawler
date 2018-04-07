#version 330 core

in vec2 uv;
in vec3 vert_normal;

out vec4 out_albedo;
out vec4 out_normal;

uniform sampler2D tex;
uniform vec2 uv_offset, uv_range;

void main() {
    out_albedo = texture(tex, uv_offset + uv*uv_range);
    out_normal = vec4(vert_normal, 1);
    if(out_albedo.a < 0.5) {
        discard;
    }
}
