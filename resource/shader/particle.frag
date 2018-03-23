#version 330 core

in vec2 uv;
in vec2 uv2;
in float progress;
in float max_frames;
out vec4 color;

uniform sampler2D tex;

void main() {
    float alpha = (max_frames * progress) - floor(max_frames * progress);
    
    color = (1 - alpha) * texture(tex, uv);
    color += alpha * texture(tex, uv2);    
}
