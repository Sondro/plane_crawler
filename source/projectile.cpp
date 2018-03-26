enum {
    PROJECTILE_FIRE,
    MAX_PROJECTILE
};

struct {
    i32 particle_type;
} projectile_data[MAX_PROJECTILE] = {
    { PARTICLE_FIRE },
};

struct ParticleUpdate {
    v2 pos,
       vel;
   
    r32 strength;
};

struct ProjectileSet {
    i32 count;
    i16 type[MAX_PROJECTILE_COUNT];
    ParticleUpdate update[MAX_PROJECTILE_COUNT];
};
