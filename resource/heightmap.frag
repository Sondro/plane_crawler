#version 330 core

in vec3 vert_normal;
in vec3 vert_color;

out vec4 color;

void main() {
    vec3 light_vector = vec3(1, -1, 1);
    float light_factor = dot(normalize(vert_normal), normalize(light_vector));

    light_factor = 0.5 + light_factor*0.5;

    color = vec4(vert_color * light_factor, 1);
    //color = vec4(vec3(1, 0, 0) * light_factor, 1);
    //color = vec4(vert_normal, 1);
}
