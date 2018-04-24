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

global
struct {
    i32 particle_type;
    r32 strength, knockback, range;
    v3 color;
} projectile_data[MAX_PROJECTILE] = {
    { PARTICLE_melee,       1, 1, 0.8f, v3(0, 0, 0) },
    { PARTICLE_fire,       1, 1, -1, v3(1, 0.7, 0.4) },
    { PARTICLE_lightning,  1, 1, -1, v3(1, 1, 0.2) },
    { PARTICLE_ice,        1, 1, -1, v3(0.2, 0.8, 1) },
    { PARTICLE_wind,       1, 1, -1, v3(0, 0.4, 0) },
    { PARTICLE_jelly,      1, 1, -1, v3(0, 0.5, 0) },
    { PARTICLE_dark,       1, 1, -1, v3(0.3, 0, 0.3) },
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
