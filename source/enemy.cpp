#define MAX_ENEMY_COUNT 256

enum {
    ENEMY_jelly,
    ENEMY_skeleton,
    ENEMY_spirit,
    MAX_ENEMY
};

struct EnemySet {
    u32 count;
    i32               id[MAX_ENEMY_COUNT];
    i32               type[MAX_ENEMY_COUNT];
    r32               anim_wait[MAX_ENEMY_COUNT];
    BoxComponent      box[MAX_ENEMY_COUNT];
    SpriteComponent   sprite[MAX_ENEMY_COUNT];
    HealthComponent   health[MAX_ENEMY_COUNT];
    AttackComponent   attack[MAX_ENEMY_COUNT];
    AIComponent       ai[MAX_ENEMY_COUNT];
    DebuffComponent   debuff[MAX_ENEMY_COUNT];
};

global
struct {
    i32 tx;
    i32 attack_type;
    r32 speed, defense;
} enemy_data[MAX_ENEMY] = {
    { 0, ATTACK_jelly, 1, 0.0 },
    { 1, ATTACK_melee, 2.5, 0.2 },
    { 2, ATTACK_dark,  1, 0.5 },
};

void init_enemy(i32 type, v2 pos,
                i32 *types, 
                BoxComponent *b, SpriteComponent *s, HealthComponent *h, AttackComponent *a, AIComponent *ai, DebuffComponent *db) {
    *types = type;
    *b = init_box_component(pos, v2(0.8, 0.8));
    *s = init_sprite_component(TEX_enemy, enemy_data[type].tx*24, 0, 24, 24);
    *h = init_health_component(1);
    *a = init_attack_component(enemy_data[type].attack_type);
    *ai = init_ai_component(AI_STATE_roam, a->type);
    *db = init_debuff_component();
}

void remove_enemy(EnemySet *e, i32 id) {
    foreach(i, e->count) {
        if(e->id[i] == id) {
            memmove(e->id + i, e->id + i+1, sizeof(i32) * (e->count - i - 1));
            memmove(e->type + i, e->type + i+1, sizeof(i32)*(e->count - i - 1));
            memmove(e->anim_wait + i, e->anim_wait + i+1, sizeof(r32) * (e->count - i -1));
            memmove(e->box + i, e->box + i+1, sizeof(BoxComponent) * (e->count - i - 1));
            memmove(e->sprite + i, e->sprite + i+1, sizeof(SpriteComponent) * (e->count - i - 1));
            memmove(e->health + i, e->health + i+1, sizeof(HealthComponent) * (e->count - i - 1));
            memmove(e->attack + i, e->attack + i+1, sizeof(AttackComponent) * (e->count - i - 1));
            memmove(e->ai + i, e->ai + i+1, sizeof(AIComponent) * (e->count - i - 1));
            memmove(e->debuff + i, e->debuff + i+1, 
                    sizeof(DebuffComponent) * (e->count -  i - 1));
            --e->count;
            break;
        }
    }
}
