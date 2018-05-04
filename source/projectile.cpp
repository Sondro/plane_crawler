#define MAX_PROJECTILE_COUNT 256

enum {
    PROJECTILE_melee,
    PROJECTILE_fire,
    PROJECTILE_lightning,
    PROJECTILE_ice,
    PROJECTILE_wind,
    PROJECTILE_jelly,
    PROJECTILE_dark,
    MAX_PROJECTILE
};

enum {
    EFFECT_dot,
    EFFECT_aoe,
    EFFECT_slow,
    EFFECT_knockback,
    MAX_EFFECT
};

global
struct {
    i32 particle_type;
    r32 strength, knockback, range;
    v3 color;
    i32 effect_type;
} projectile_data[MAX_PROJECTILE] = {
    { PARTICLE_melee,      2, 1, 0.8f, v3(0, 0, 0), EFFECT_knockback },
    { PARTICLE_fire,       1, 1, -1, v3(1, 0.7, 0.4), EFFECT_dot },
    { PARTICLE_lightning,  1, 1, -1, v3(1, 1, 0.2), EFFECT_aoe },
    { PARTICLE_ice,        1, 1, -1, v3(0.2, 0.8, 1), EFFECT_slow },
    { PARTICLE_wind,       1, 1, -1, v3(0, 0.4, 0), EFFECT_knockback },
    { PARTICLE_jelly,      1, 1, -1, v3(0, 0.5, 0), EFFECT_slow  },
    { PARTICLE_dark,       1, 1, -1, v3(0.3, 0, 0.3), EFFECT_dot },
};

struct ProjectileUpdate {
    i32 origin;
    v2 pos, vel;
    r32 strength, knockback;
    r32 particle_start_time, range_sq, distance_traveled_sq;
};

struct ProjectileSet {
    i32 count;
    i16 type[MAX_PROJECTILE_COUNT];
    ProjectileUpdate update[MAX_PROJECTILE_COUNT];
};
