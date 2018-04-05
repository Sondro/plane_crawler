#define MAX_LIGHT 32

struct Light {
    v3 pos, color;
    r32 radius, intensity;
};

struct LightState {
    i32 light_count;
    Light lights[MAX_LIGHT];
};

void init_light_state(LightState *l) {
    l->light_count = 0;
}
