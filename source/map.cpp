enum {
    TILE_BRICK,
    TILE_BRICK_WALL,
    TILE_DIRT,
    TILE_WATER,
    TILE_PIT,
    MAX_TILE
};

struct Map {
    i8 tile[MAP_W][MAP_H];
};
