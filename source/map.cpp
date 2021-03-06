enum {
    DUNGEON_TYPE_easy,
    DUNGEON_TYPE_medium,
    DUNGEON_TYPE_hard,
};

#define MAP_W 256
#define MAP_H 256

#define TILE_SET_TILE_SIZE 32
#define WALL    0x01
#define PIT     0x02
#define DOOR    0x04
#define LADDER  0x08

void calculate_heightmap_normal(r32 *verts, r32 *norms) {
    v3 vert1 = v3(verts[3]-verts[0], verts[4]-verts[1], verts[5]-verts[2]),
    vert2 = v3(verts[6]-verts[0], verts[7]-verts[1], verts[8]-verts[2]);
    
    v3 normal = HMM_Cross(vert1, vert2);
    normal /= (HMM_Length(vert1)*HMM_Length(vert2));
    
    if(normal.y < 0) {
        normal *= -1;
    }
    
    norms[0] = normal.x;
    norms[1] = normal.y;
    norms[2] = normal.z;
    norms[3] = normal.x;
    norms[4] = normal.y;
    norms[5] = normal.z;
    norms[6] = normal.x;
    norms[7] = normal.y;
    norms[8] = normal.z;
}

#include "dungeon_map.cpp"
#include "house_map.cpp"
