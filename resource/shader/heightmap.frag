#version 330 core

in vec2 uv;
in vec3 vert_normal;
out vec4 color;

uniform sampler2D tex;

void main() {
    vec3 light_vector = vec3(1, -1, 1);

    float light_factor = dot(normalize(vert_normal), normalize(light_vector));
    light_factor = 0.6 + light_factor*0.5;
    
    if(light_factor > 1) light_factor = 1;

    vec3 albedo = texture(tex, uv).rgb;

    color = vec4(albedo*light_factor, 1);
}
