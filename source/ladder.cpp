#define MAX_LADDER_COUNT 32

struct LadderSet {
    u64 count;
    i32 x[MAX_LADDER_COUNT];
    i32 y[MAX_LADDER_COUNT];
    SpriteComponent sprite[MAX_LADDER_COUNT];
};

void init_ladder_set(LadderSet *l) {
    l->count = 0;
}

void add_ladder(LadderSet *l, i32 x, i32 y) {
    if(l->count < MAX_LADDER_COUNT) {
        l->x[l->count] = x;
        l->y[l->count] = y;
        l->sprite[l->count] = init_sprite_component(TEX_tile_dungeon, 64, 32, 32, 32);
        l->sprite[l->count++].pos = v3(x+0.5, 0, y+0.5);
    }
}