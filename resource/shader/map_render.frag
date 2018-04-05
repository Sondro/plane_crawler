#version 330 core

struct Light {
	vec3 pos, color;
	float intensity, radius;
};

in vec2 uv;
out vec4 color;
out float gl_FragDepth;

uniform mat4 projection3d;
uniform mat4 view3d;
uniform mat4 model;

uniform sampler2D albedo;
uniform sampler2D normal;
uniform sampler2D depth;

uniform float brightness;
uniform Light lights[32];
uniform int light_count;
uniform vec3 light_vector;

void main(void) {
	vec2 tex_uv = vec2(uv.x, 1-uv.y);
	
	vec3 albedo = texture(albedo, tex_uv).rgb;
	vec3 normal = texture(normal, tex_uv).rgb;
	float depth = texture(depth, tex_uv).r;

	vec4 screen_coord4 = vec4(vec3(tex_uv.x*2 - 1, tex_uv.y*2 - 1, depth*2 - 1), 1.0);
	vec4 world_coord4 = inverse(view3d) * inverse(projection3d) * screen_coord4;
	vec3 position = world_coord4.xyz / world_coord4.w;

	float diffuse = clamp(dot(normalize(light_vector), normalize(normal)), 0.1, 1.5);

	color = vec4(albedo, 1);
	color.xyz *= brightness;
	color.xyz *= diffuse;

	vec4 original_color = color;
	vec3 light_addition = vec3(0, 0, 0);
	bool found_light = false;
	for(int i = 0; i < light_count && i < 32; ++i) {
		float distance = distance(position, lights[i].pos);
		if(distance < lights[i].radius) {
			found_light = true;
			vec3 light_direction = normalize(lights[i].pos - position);
			float light_amount = 1.0 - clamp((distance / lights[i].radius), 0, 1);
			light_addition += (light_amount * lights[i].intensity * lights[i].color * clamp(dot(light_direction, normalize(normal)), 0, 1));
		}
	}
	if(found_light) {
		color.xyz /= brightness;
		color.xyz /= diffuse;
	}

	if(light_addition.r > 1.5) light_addition.r = 1.5;
	if(light_addition.g > 1.5) light_addition.g = 1.5;
	if(light_addition.b > 1.5) light_addition.b = 1.5;
	color.xyz *= light_addition;
	if(color.r < original_color.r) color.r = original_color.r;
	if(color.g < original_color.g) color.g = original_color.g;
	if(color.b < original_color.b) color.b = original_color.b;

	gl_FragDepth = depth;
}
