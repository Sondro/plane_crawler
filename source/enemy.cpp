enum {
    ENEMY_SKELETON,
    ENEMY_SPIRIT,
    ENEMY_JELLY,
    MAX_ENEMY
};

struct EnemySet {
    u32 count;
    BoxComponent      box[MAX_ENEMY_COUNT];
    SpriteComponent   sprite[MAX_ENEMY_COUNT];
    HealthComponent   health[MAX_ENEMY_COUNT];
    AttackComponent   attack[MAX_ENEMY_COUNT];
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

void init_enemy(i16 type, v2 pos,
                BoxComponent *b, SpriteComponent *s, HealthComponent *h, AttackComponent *a) {
    
    *b = init_box_component(pos, v2(0.4, 0.4));
    *s = init_sprite_component(TEX_ENEMY, enemy_data[type].tx*16, 0, 16, 16);
    *h = init_health_component(1);
    *a = init_attack_component(ATTACK_FIREBALL);
}
