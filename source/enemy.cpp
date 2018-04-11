#define MAX_ENEMY_COUNT 256

enum {
    ENEMY_skeleton,
    ENEMY_spirit,
    ENEMY_jelly,
    MAX_ENEMY
};

struct EnemySet {
    u32 count;
    i32               id[MAX_ENEMY_COUNT];
    BoxComponent      box[MAX_ENEMY_COUNT];
    SpriteComponent   sprite[MAX_ENEMY_COUNT];
    HealthComponent   health[MAX_ENEMY_COUNT];
    AttackComponent   attack[MAX_ENEMY_COUNT];
    AIComponent       ai[MAX_ENEMY_COUNT];
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
                BoxComponent *b, SpriteComponent *s, HealthComponent *h, AttackComponent *a, AIComponent *ai) {
    *b = init_box_component(pos, v2(0.8, 0.8));
    *s = init_sprite_component(TEX_enemy, enemy_data[type].tx*24, 0, 24, 24);
    *h = init_health_component(1);
    *a = init_attack_component(ATTACK_fireball);
    *ai = init_ai_component(AI_roam);
}

void remove_enemy(EnemySet *e, i32 id) {
    foreach(i, e->count) {
        if(e->id[i] == id) {
            memmove(e->id + i, e->id + i+1, sizeof(i32) * (e->count - i - 1));
            memmove(e->box + i, e->box + i+1, sizeof(BoxComponent) * (e->count - i - 1));
            memmove(e->sprite + i, e->sprite + i+1, sizeof(SpriteComponent) * (e->count - i - 1));
            memmove(e->health + i, e->health + i+1, sizeof(HealthComponent) * (e->count - i - 1));
            memmove(e->attack + i, e->attack + i+1, sizeof(AttackComponent) * (e->count - i - 1));
            memmove(e->ai + i, e->ai + i+1, sizeof(AIComponent) * (e->count - i - 1));
            --e->count;
            break;
        }
    }
}
