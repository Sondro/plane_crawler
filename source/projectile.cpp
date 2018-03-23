enum {
    PROJECTILE_FIRE,
    MAX_PROJECTILE
};

struct {
    i32 particle_type;
} projectile_data[MAX_PROJECTILE] = {
    { PARTICLE_FIRE },
};

struct ProjectileSet {
    i32 count;
    i16 type[MAX_PROJECTILE_COUNT];
    v4 pos_vel[MAX_PROJECTILE_COUNT];
};
