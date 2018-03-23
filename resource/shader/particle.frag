#version 330 core

in vec2 uv;
in vec2 uv2;
in float progress;
in float max_frames;
out vec4 color;

uniform sampler2D tex;
uniform vec4 clip;
uniform vec4 tint;

void main() {
	color = vec4(0, 0, 0, 0);
    if(gl_FragCoord.x >= clip.x && gl_FragCoord.x <= clip.z &&
       gl_FragCoord.y >= clip.y && gl_FragCoord.y <= clip.w) {
        float alpha = (max_frames * progress) - floor(max_frames * progress);
		
		color = (1 - alpha) * texture(tex, uv);
		color += alpha * texture(tex, uv2);
		
		color *= tint;
    }
    else {
        discard;
    }
}
