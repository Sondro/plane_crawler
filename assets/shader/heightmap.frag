#version 330 core

in vec2 uv;
in vec3 vert_normal;

out vec4 out_albedo;
out vec4 out_normal;

uniform sampler2D tex;

void main() {
    out_albedo = texture(tex, uv).rgba;
	if(out_albedo.a > 0.5) {
		out_normal = vec4(vert_normal, 1);
	}
	else {
		discard;
	}
}
