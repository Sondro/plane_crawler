#version 330 core

in vec2 uv;
in vec2 uv2;
in float progress;
in float max_frames;
out vec4 out_albedo;
out vec4 out_normal;

uniform sampler2D tex;

void main() {
    float alpha = (max_frames * progress) - floor(max_frames * progress);
    
    out_albedo = (1 - alpha) * texture(tex, uv);
    out_albedo += alpha * texture(tex, uv2);    
    out_normal = vec4(0, 0, 1, 0);
}
