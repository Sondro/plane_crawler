enum {
    ENEMY_SKELETON,
    ENEMY_SPIRIT,
    ENEMY_JELLY,
    MAX_ENEMY
};

global
struct {
    i32 tx;
    r32 defense;
} enemy_data[MAX_ENEMY] = {
    { 0, 0.2 },
    { 1, 0.5 },
    { 2, 0.0 },
};

struct EnemyUpdate {
    v2 pos, vel;
    r32 update_dir_t;
};

struct EnemySet {
    u32 count;
    i16 type[MAX_ENEMY_COUNT];
    EnemyUpdate update[MAX_ENEMY_COUNT];
};
