#define MAX_PROJECTILE_COUNT 256

enum {
    PROJECTILE_fire,
    PROJECTILE_lightning,
    PROJECTILE_ice,
    PROJECTILE_wind,
    MAX_PROJECTILE
};

global
struct {
    i32 particle_type;
} projectile_data[MAX_PROJECTILE] = {
    { PARTICLE_fire },
    { PARTICLE_lightning },
    { PARTICLE_ice },
    { PARTICLE_wind },
};

struct ProjectileUpdate {
    i32 origin;
    v2 pos,
    vel;
    r32 strength, particle_start_time;
};

struct ProjectileSet {
    i32 count;
    i16 type[MAX_PROJECTILE_COUNT];
    ProjectileUpdate update[MAX_PROJECTILE_COUNT];
};
