enum {
    PROJECTILE_FIRE,
    MAX_PROJECTILE
};

struct {
    i32 particle_type;
} projectile_data[MAX_PROJECTILE] = {
    { PARTICLE_FIRE },
};

struct ProjectileUpdate {
    i32 origin;
    v2 pos,
       vel;   
    r32 strength;
};

struct ProjectileSet {
    i32 count;
    i16 type[MAX_PROJECTILE_COUNT];
    ProjectileUpdate update[MAX_PROJECTILE_COUNT];
};
