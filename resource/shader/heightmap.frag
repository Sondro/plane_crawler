#version 330 core

in vec2 uv;
in vec3 vert_normal;
out vec4 color;

uniform sampler2D tex;
uniform vec3 light_vector;

void main() {
    float light_factor = clamp(dot(vert_normal, light_vector), 0.4, 1);
    
    if(light_factor > 1) light_factor = 1;

    vec3 albedo = texture(tex, uv).rgb;
    
    color = vec4(albedo*light_factor, 1);
}
