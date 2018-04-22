#define MAX_COLLECTIBLE_COUNT 256

// @Collectible Types

enum {
    COLLECTIBLE_health_pot,
    COLLECTIBLE_key,
    COLLECTIBLE_mana_pot,
    MAX_COLLECTIBLE
};

global struct {
    i32 tx;
} collectible_data[MAX_COLLECTIBLE] = {
    { 0 },
    { 1 },
    { 2 },
};

struct CollectibleSet {
    u32 count;
    i8 type[MAX_COLLECTIBLE_COUNT];
    BoxComponent box[MAX_COLLECTIBLE_COUNT];
    SpriteComponent sprite[MAX_COLLECTIBLE_COUNT];
};

void init_collectible(i8 type, v2 pos, i8 *type_arr, BoxComponent *b, SpriteComponent *s) {
    *type_arr = type;
    *b = init_box_component(pos, v2(0.6, 0.6));
    *s = init_sprite_component(TEX_collectible, collectible_data[type].tx*8, 0, 8, 8);
}
