#define MAX_DOOR_COUNT 64

struct DoorSet {
    u64 count;
    i32 x[MAX_DOOR_COUNT];
    i32 y[MAX_DOOR_COUNT];
    SpriteComponent sprite[MAX_DOOR_COUNT];
};

void init_door_set(DoorSet *d) {
    d->count = 0;
}

void add_door(DoorSet *d, i32 x, i32 y) {
    if(d->count < MAX_DOOR_COUNT) {
        d->x[d->count] = x;
        d->y[d->count] = y;
        d->sprite[d->count] = init_sprite_component(TEX_tile_dungeon, 96, 0, 32, 64);
        d->sprite[d->count++].pos = v3(x+0.5, 0, y+0.5);
    }
}

void remove_door(DoorSet *d, u64 i) {
    if(i >= 0 && i < d->count) {
        memmove(d->x + i, d->x + i + 1, sizeof(i32)*(d->count - i - 1));
        memmove(d->y + i, d->y + i + 1, sizeof(i32)*(d->count - i - 1));
        memmove(d->sprite + i, d->sprite + i + 1, sizeof(SpriteComponent)*(d->count - i - 1));
        --d->count;
    }
}