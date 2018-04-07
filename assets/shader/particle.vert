#version 330 core

in vec3 in_position;
in vec2 in_uv;
in mat4 model;
in vec2 particle_data; // == progress, max_frames

out vec2 uv;
out vec2 uv2;

out float progress;
out float max_frames;

uniform mat4 projection;
uniform mat4 view;

vec2 get_uv_offset(int tex) {
    if(tex < 4) {
        return(vec2(float(tex) / 4, 0));
    }
    else if(tex >= 4 && tex < 8) {
        return(vec2(float(tex - 4) / 4, 0.25));
    }
    else if(tex >= 8 && tex < 12) {
        return(vec2(float(tex - 8) / 4, 0.5));
    }
    return(vec2(float(tex - 12) / 4, 0.75));
}

void main() {
    uv = in_uv;
    uv /= 4;
    
    uv2 = uv;
    
    uv += get_uv_offset(int(floor(float(particle_data.y) * particle_data.x)));
    uv2 += get_uv_offset(int(floor(float(particle_data.y) * particle_data.x)) + 1);
    
    gl_Position = projection * view * model * vec4(in_position, 1.0); 
    
    progress = particle_data.x;
    max_frames = particle_data.y;
}
