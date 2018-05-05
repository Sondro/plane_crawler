#include "enemy.cpp"
#include "collectible.cpp"
#include "prefab_rooms.cpp"
#include "door.cpp"
#include "ladder.cpp"

enum {
    DUNGEON_TILE_brick,
    DUNGEON_TILE_brick_wall,
    DUNGEON_TILE_dirt,
    DUNGEON_TILE_broken_stone,
    DUNGEON_TILE_pit,
    DUNGEON_TILE_door,
    DUNGEON_TILE_ladder,
    MAX_DUNGEON_TILE
};

global
struct {
    i8 tx, ty,
    flags;
} dungeon_tile_data[MAX_DUNGEON_TILE] = {
    { 0, 0, 0 },
    { 0, 0, WALL },
    { 1, 0, 0 },
    { 1, 1, 0 },
    { 1, 0, PIT },
    { 1, 1, DOOR },
    { 1, 0, PIT },
};

struct DungeonMap {
    i8 tiles[MAP_W][MAP_H];
    r32 heights[MAP_W+1][MAP_H+1];
    
    LadderSet       ladders;
    DoorSet         doors;
    EnemySet        enemies;
    CollectibleSet  collectibles;
    
    ProjectileSet projectiles;
    ParticleMaster particles;
    LightState lighting;
    
    v3 light_vector;
    
    u64 vertex_component_count;
    GLuint vao,
    vertex_vbo,
    uv_vbo,
    normal_vbo;
    
    GBuffer render;
};

void request_dungeon_map_assets() {
    request_shader(SHADER_heightmap);
    request_shader(SHADER_world_texture);
    request_shader(SHADER_world_render);
    request_shader(SHADER_particle);
    request_texture(TEX_tile_dungeon);
    request_texture(TEX_enemy);
    request_texture(TEX_collectible);
    request_sound(SOUND_key_get);
    request_sound(SOUND_spell_fire);
    
    foreach(i, MAX_PARTICLE) {
        request_texture(particle_types[i].texture);
    }
}

void unrequest_dungeon_map_assets() {
    unrequest_shader(SHADER_heightmap);
    unrequest_shader(SHADER_world_texture);
    unrequest_shader(SHADER_world_render);
    unrequest_shader(SHADER_particle);
    unrequest_texture(TEX_tile_dungeon);
    unrequest_texture(TEX_enemy);
    unrequest_texture(TEX_collectible);
    unrequest_sound(SOUND_key_get);
    unrequest_sound(SOUND_spell_fire);
    
    foreach(i, MAX_PARTICLE) {
        unrequest_texture(particle_types[i].texture);
    }
}

r32 dungeon_coordinate_height(DungeonMap *d, r32 x, r32 z) {
    if(x >= 0 && x < MAP_W-1 &&
       z >= 0 && z < MAP_H-1) {
        int height_x = (int)x,
        height_z = (int)z;
        
        v3 q00, q10,
        q01, q11;
        
        q00 = v3(0, d->heights[height_x][height_z], 0);
        q10 = v3(1, d->heights[height_x+1][height_z], 0);
        q01 = v3(0, d->heights[height_x][height_z+1], 1);
        q11 = v3(1, d->heights[height_x+1][height_z+1], 1);
        
        v3 p = v3(x - height_x, 0, z - height_z);
        
        r32 side_0 = q00.y + p.z*(q01.y - q00.y),
        side_1 = q10.y + p.z*(q11.y - q10.y);
        
        p.y = side_0 + p.x*(side_1 - side_0);
        
        return p.y;
    }
    return 0.f;
}

void do_particle(DungeonMap *d, i8 type, v3 pos, v3 vel, r32 scale) {
    do_particle(&d->particles, type, pos, vel, scale);
}

void add_enemy(DungeonMap *d, i16 type, v2 pos) {
    if(d->enemies.count < MAX_ENEMY_COUNT) {
        u32 it = d->enemies.count;
        
        i32 max_id = 1;
        foreach(i, d->enemies.count) {
            if(d->enemies.id[i] >= max_id) {
                max_id = d->enemies.id[i]+1;
            }
        }
        
        d->enemies.id[it] = max_id;
        d->enemies.anim_wait[it] = 0;
        init_enemy(type, pos,
                   d->enemies.type + it,
                   d->enemies.box + it,
                   d->enemies.sprite + it,
                   d->enemies.health + it,
                   d->enemies.attack + it,
                   d->enemies.ai + it,
                   d->enemies.debuff + it);
        
        ++d->enemies.count;
    }
}

void remove_enemy(DungeonMap *d, i32 id) {
    remove_enemy(&d->enemies, id);
}

void add_collectible(DungeonMap *d, i8 type, v2 pos) {
    if(d->collectibles.count < MAX_COLLECTIBLE_COUNT) {
        u32 i = d->collectibles.count;
        init_collectible(type, pos, d->collectibles.type + i, d->collectibles.box + i, d->collectibles.sprite + i);
        ++d->collectibles.count;
    }
}

void remove_collectible(DungeonMap *d, u32 i) {
    if(i >= 0 && i < d->collectibles.count) {
        memmove(d->collectibles.type + i, d->collectibles.type + i + 1, sizeof(i8) * (d->collectibles.count - i - 1));
        memmove(d->collectibles.box + i, d->collectibles.box + i + 1, sizeof(BoxComponent) * (d->collectibles.count - i - 1));
        memmove(d->collectibles.sprite + i, d->collectibles.sprite + i + 1, sizeof(SpriteComponent) * (d->collectibles.count - i - 1));
        --d->collectibles.count;
    }
}

void add_projectile(DungeonMap *d, i32 origin, i16 type, v2 pos, v2 vel, r32 strength) {
    if(d->projectiles.count < MAX_PROJECTILE_COUNT) {
        d->projectiles.type[d->projectiles.count] = type;
        d->projectiles.update[d->projectiles.count].origin = origin;
        d->projectiles.update[d->projectiles.count].pos = pos;
        d->projectiles.update[d->projectiles.count].vel = vel;
        d->projectiles.update[d->projectiles.count].strength = strength;
        d->projectiles.update[d->projectiles.count].particle_start_time = current_time;
        d->projectiles.update[d->projectiles.count].range_sq = projectile_data[type].range < 0 ? -1 : projectile_data[type].range*projectile_data[type].range;
        d->projectiles.update[d->projectiles.count].distance_traveled_sq = 0.f;
        ++d->projectiles.count;
    }
}

void remove_projectile(DungeonMap *d, i32 i) {
    if(i >= 0 && i < d->projectiles.count) {
        foreach(j, 100) {
            r32 pitch = random32(0, PI/2),
            yaw = random32(0, 2*PI);
            do_particle(d,
                        projectile_data[d->projectiles.type[i]].particle_type,
                        v3(d->projectiles.update[i].pos.x,
                           dungeon_coordinate_height(d, d->projectiles.update[i].pos.x, d->projectiles.update[i].pos.y) + 0.5,
                           d->projectiles.update[i].pos.y),
                        v3(cos(yaw)*60, sin(pitch)*60, sin(yaw)*60) / random32(20, 40),
                        random32(0.05, 0.15 + d->projectiles.update[i].strength*0.25));
        }
        
        memmove(d->projectiles.type+i, d->projectiles.type+i+1, sizeof(i16) * (d->projectiles.count - i - 1));
        memmove(d->projectiles.update+i, d->projectiles.update+i+1, sizeof(ProjectileUpdate) * (d->projectiles.count - i - 1));
        --d->projectiles.count;
    }
}

void add_door(DungeonMap *d, i32 x, i32 y) {
    add_door(&d->doors, x, y);
    d->doors.sprite[d->doors.count-1].pos.y = dungeon_coordinate_height(d, x + 0.5, y + 0.5);
}

void generate_dungeon_map(DungeonMap *d) {
    init_ladder_set(&d->ladders);
    init_door_set(&d->doors);
    d->enemies.count = 0;
    d->collectibles.count = 0;
    d->projectiles.count = 0;
    init_particle_master(&d->particles);
    
    d->light_vector = v3(2, 1, 1) / sqrt(6);
    
    // @Room generation
    {
        foreach(i, MAP_W)
            foreach(j, MAP_H) {
            d->tiles[i][j] = DUNGEON_TILE_brick_wall;
            d->heights[i][j] = perlin_2d(i, j, 0.15, 12)*1.1;
        }
        
        i32 room_count = 10;
        
        i16 current_room = 1;
        
        struct Room {
            i8 origin_dir, branch_depth;
            i16 x, y, w, h, ground_tile;
            i16 prefab_index;
        };
        Room *rooms = heap_alloc(Room, room_count);
        
        rooms[0].origin_dir = -1;
        rooms[0].branch_depth = 0;
        rooms[0].x = MAP_W/2;
        rooms[0].y = MAP_H/2;
        rooms[0].w = 8;
        rooms[0].h = 8;
        rooms[0].prefab_index = 0;
        rooms[0].ground_tile = DUNGEON_TILE_brick;
        
        i32 ladders_spawned = 0;
        
        foreach(i, current_room) {
            if(current_room < room_count && rooms[i].branch_depth < 6) {
                foreach(dir, 4) {
                    if(rooms[i].origin_dir != (int)dir && (random32(0, 1) < 0.3 || !i)) {
                        rooms[current_room].origin_dir = !i ? is_even(dir) ? dir+1 : dir-1 : rooms[i].origin_dir;
                        rooms[current_room].branch_depth = rooms[i].branch_depth + 1;
                        
                        if(random32(0, 1) < 0.5f) {
                            rooms[current_room].prefab_index = (int)random32(0, sizeof(prefab_rooms)/sizeof(prefab_rooms[0]) - 1) + 1;
                            
                            const char *prefab_room_str = prefab_rooms[rooms[current_room].prefab_index];
                            
                            i8 found_first_newline = 0;
                            rooms[current_room].w = 0;
                            rooms[current_room].h = 0;
                            for(u64 j = 0; prefab_room_str[j]; ++j) {
                                if(prefab_room_str[j] == '\n') {
                                    if(!found_first_newline) {
                                        rooms[current_room].w = j;
                                        found_first_newline = 1;
                                    }
                                    ++rooms[current_room].h;
                                }
                            }
                        }
                        else {
                            rooms[current_room].w = random32(4, 16);
                            rooms[current_room].h = random32(rooms[current_room].w-3, rooms[current_room].w+3);
                        }
                        
                        rooms[current_room].ground_tile = random32(0, 1) < 0.5 ? DUNGEON_TILE_broken_stone : DUNGEON_TILE_dirt;
                        
                        i16 x_change = 0, y_change = 0;
                        switch(dir) {
                            case 0: { // right
                                x_change = rooms[i].w/2 + rooms[current_room].w/2 + random32(3, 10);
                                y_change = 0;
                                break;
                            }
                            case 1: { // left
                                x_change = -(rooms[i].w/2 + rooms[current_room].w/2 + random32(3, 10));
                                y_change = 0;
                                break;
                            }
                            case 2: { // up
                                x_change = 0;
                                y_change = -(rooms[i].h/2 + rooms[current_room].h/2 + random32(3, 10));
                                break;
                            }
                            case 3: { // down
                                x_change = 0;
                                y_change = rooms[i].h/2 + rooms[current_room].h/2 + random32(3, 10);
                                break;
                            }
                            default: break;
                        }
                        
                        rooms[current_room].x = rooms[i].x + x_change;
                        rooms[current_room].y = rooms[i].y + y_change;
                        
                        i16 hall_x = min(rooms[current_room].x, rooms[i].x);
                        i16 hall_y = min(rooms[current_room].y, rooms[i].y);
                        
                        forrng(x, hall_x, hall_x + abs(x_change)+1) {
                            forrng(y, hall_y, hall_y + abs(y_change)+1) {
                                if(x >= 1 && x < MAP_W-1 && y >= 1 && y < MAP_H-1) {
                                    d->tiles[x][y] = DUNGEON_TILE_broken_stone;
                                }
                            }
                        }
                        
                        if(++current_room >= room_count) {
                            break;
                        }
                    }
                }
            }
        }
        
        for(int i = room_count-1; i>=0; --i) {
            forrng(x, rooms[i].x - rooms[i].w/2, rooms[i].x + rooms[i].w/2)
                forrng(y, rooms[i].y - rooms[i].h/2, rooms[i].y + rooms[i].h/2) {
                if(x >= 1 && x < MAP_W-1 && y >= 1 && y < MAP_H-1) {
                    if(x < rooms[i].x + rooms[i].w && x < MAP_W-2 &&
                       y < rooms[i].y + rooms[i].h && y < MAP_H-2) {
                        if(rooms[i].prefab_index > 0 && rooms[i].prefab_index < (i16)(sizeof(prefab_rooms)/sizeof(prefab_rooms[0]))) {
                            i32 room_x = x - (rooms[i].x - rooms[i].w/2);
                            i32 room_y = y - (rooms[i].y - rooms[i].h/2);
                            u64 tile_i = room_y*(rooms[i].w+1) + room_x;
                            
                            char tile_char = 
                                tile_i < strlen(prefab_rooms[rooms[i].prefab_index]) &&
                                room_x >= 0 && room_x < MAP_W &&
                                room_y >= 0 && room_y < MAP_H ? 
                            
                                prefab_rooms[rooms[i].prefab_index][tile_i] 
                            
                                : 
                            
                            0;
                            
                            switch(tile_char) {
                                // NOTE(Ryan): Wall
                                case '#': {
                                    d->tiles[x][y] = DUNGEON_TILE_brick_wall;
                                    break;
                                }
                                // NOTE(Ryan): Ground
                                case ' ': {
                                    d->tiles[x][y] = rooms[i].ground_tile;
                                    break;
                                }
                                // NOTE(Ryan): Pit
                                case 'O': {
                                    d->tiles[x][y] = DUNGEON_TILE_pit;
                                    break;
                                }
                                // NOTE(Ryan): Door
                                case '%': {
                                    d->tiles[x][y] = DUNGEON_TILE_door;
                                    break;
                                }
                                // NOTE(Ryan): Key
                                case '$': {
                                    add_collectible(d, COLLECTIBLE_key, v2(x+0.5f, y+0.5f));
                                    break;
                                }
                                // NOTE(Ryan): Consumable
                                case '+': {
                                    if(random32(0, 1) < 0.5) {
                                        add_collectible(d, COLLECTIBLE_mana_pot, v2(x+0.5f, y+0.5f));
                                    }
                                    else {
                                        add_collectible(d, COLLECTIBLE_health_pot, v2(x+0.5f, y+0.5f));
                                    }
                                    break;
                                }
                                // NOTE(Ryan): Trophy
                                case '&': {
                                    break;
                                }
                                // NOTE(Ryan): Ladder
                                case 'L': {
                                    d->tiles[x][y] = DUNGEON_TILE_ladder;
                                    ++ladders_spawned;
                                    break;
                                }
                                // NOTE(Ryan): Skeleton
                                case 'S': {
                                    add_enemy(d, ENEMY_skeleton, v2(x+0.5f, y+0.5f));
                                    break;
                                }
                                // NOTE(Ryan): Jelly
                                case 'J': {
                                    add_enemy(d, ENEMY_jelly, v2(x+0.5f, y+0.5f));
                                    break;
                                }
                                // NOTE(Ryan): Ghost
                                case 'G': {
                                    add_enemy(d, ENEMY_spirit, v2(x+0.5f, y+0.5f));
                                    break;
                                }
                                default: break;
                            }
                        }
                        else {
                            d->tiles[x][y] = rooms[i].ground_tile;
                            if(i) {
                                r32 random_val = random32(0, 1);
                                if(random_val < 0.025) {
                                    add_enemy(d, ENEMY_jelly, v2(x+0.5f, y+0.5f));
                                }
                                else if(random_val < 0.05) {
                                    add_collectible(d, COLLECTIBLE_health_pot, v2(x+0.5f, y+0.5f));
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if(!ladders_spawned) {
            forrng(i, room_count/2, room_count) {
                if(random32(0, 1) < 0.3 || (i == room_count-1 && !ladders_spawned)) {
                    i32 place_x = rooms[i].x;
                    i32 place_y = rooms[i].y;
                    if(place_x >= 0 && place_x < MAP_W &&
                       place_y >= 0 && place_y < MAP_H) {
                        d->tiles[place_x][place_y] = DUNGEON_TILE_ladder;
                        
                        if(++ladders_spawned > 2) {
                            break;
                        }
                    }
                }
            }
        }
        
        free(rooms);
    }
    
    foreach(i, MAP_W) {
        foreach(j, MAP_H) {
            if(d->tiles[i][j] == DUNGEON_TILE_door) {
                add_door(d, i, j);
            }
            else if(d->tiles[i][j] == DUNGEON_TILE_ladder) {
                add_ladder(&d->ladders, i, j);
            }
        }
    }
    
    //
    
    r32 *vertices = 0,
    *uvs = 0,
    *normals = 0;
    
    u32 x = 0,
    z = 0;
    
    while(1) {
        v3 v00 = v3(x,   d->heights[x][z],     z);
        v3 v01 = v3(x+1, d->heights[x+1][z],   z);
        v3 v10 = v3(x,   d->heights[x][z+1],   z+1);
        v3 v11 = v3(x+1, d->heights[x+1][z+1], z+1);
        
        r32 tx = (dungeon_tile_data[d->tiles[x][z]].tx*(r32)TILE_SET_TILE_SIZE)/(r32)textures[TEX_tile_dungeon].w,
        ty = (dungeon_tile_data[d->tiles[x][z]].ty*(r32)TILE_SET_TILE_SIZE)/(r32)textures[TEX_tile_dungeon].h,
        tw = (r32)TILE_SET_TILE_SIZE/textures[TEX_tile_dungeon].w,
        th = (r32)TILE_SET_TILE_SIZE/textures[TEX_tile_dungeon].h;
        
        if(dungeon_tile_data[d->tiles[x][z]].flags & WALL) {
            if(x && !(dungeon_tile_data[d->tiles[x-1][z]].flags & WALL)) {
                foreach(height, 3) {
                    if(random32(0, 1) < 0.2) {
                        ty = 32.f/textures[TEX_tile_dungeon].w;
                    }
                    else {
                        ty = 0;
                    }
                    
                    r32 verts[] = {
                        v00.x, v00.y+height, v00.z,
                        v10.x, v10.y+height, v10.z,
                        v00.x, v00.y+height+1, v00.z,
                        
                        v10.x, v10.y+height+1, v10.z,
                        v00.x, v00.y+height+1, v00.z,
                        v10.x, v10.y+height, v10.z
                    };
                    
                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,
                        
                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };
                    
                    r32 norms[18] = { 0 };
                    
                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);
                    
                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(x<MAP_W-1 && !(dungeon_tile_data[d->tiles[x+1][z]].flags & WALL)) {
                foreach(height, 3) {
                    if(random32(0, 1) < 0.2) {
                        ty = 32.f/textures[TEX_tile_dungeon].w;
                    }
                    else {
                        ty = 0;
                    }
                    
                    r32 verts[] = {
                        v01.x, v01.y+height, v01.z,
                        v11.x, v11.y+height, v11.z,
                        v01.x, v01.y+height+1, v01.z,
                        
                        v11.x, v11.y+height+1, v11.z,
                        v01.x, v01.y+height+1, v01.z,
                        v11.x, v11.y+height, v11.z
                    };
                    
                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,
                        
                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };
                    
                    r32 norms[18] = { 0 };
                    
                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);
                    
                    foreach(i, 18) {
                        norms[i] *= -1;
                    }
                    
                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(z && !(dungeon_tile_data[d->tiles[x][z-1]].flags & WALL)) {
                foreach(height, 3) {
                    if(random32(0, 1) < 0.2) {
                        ty = 32.f/textures[TEX_tile_dungeon].w;
                    }
                    else {
                        ty = 0;
                    }
                    
                    r32 verts[] = {
                        v00.x, v00.y+height, v00.z,
                        v01.x, v01.y+height, v01.z,
                        v00.x, v00.y+height+1, v00.z,
                        
                        v01.x, v01.y+height+1, v01.z,
                        v00.x, v00.y+height+1, v00.z,
                        v01.x, v01.y+height, v01.z
                    };
                    
                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,
                        
                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };
                    
                    r32 norms[18] = { 0 };
                    
                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);
                    
                    foreach(i, 18) {
                        norms[i] *= -1;
                    }
                    
                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(z < MAP_H-1 && !(dungeon_tile_data[d->tiles[x][z+1]].flags & WALL)) {
                foreach(height, 3) {
                    if(random32(0, 1) < 0.2) {
                        ty = 32.f/textures[TEX_tile_dungeon].w;
                    }
                    else {
                        ty = 0;
                    }
                    
                    r32 verts[] = {
                        v10.x, v10.y+height, v10.z,
                        v11.x, v11.y+height, v11.z,
                        v10.x, v10.y+height+1, v10.z,
                        
                        v11.x, v11.y+height+1, v11.z,
                        v10.x, v10.y+height+1, v10.z,
                        v11.x, v11.y+height, v11.z
                    };
                    
                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,
                        
                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };
                    
                    r32 norms[18] = { 0 };
                    
                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);
                    
                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
        }
        else if(dungeon_tile_data[d->tiles[x][z]].flags & PIT) {
            forrng(k, 1, 2) {
                r32 verts[] = {
                    v00.x, v00.y + k*3, v00.z,
                    v01.x, v01.y + k*3, v01.z,
                    v10.x, v10.y + k*3, v10.z,
                    
                    v11.x, v11.y + k*3, v11.z,
                    v01.x, v01.y + k*3, v01.z,
                    v10.x, v10.y + k*3, v10.z,
                };
                
                if(k) {
                    tx = 64.f / textures[TEX_tile_dungeon].w,
                    ty = 0.f;
                }
                
                r32 uvs_[] = {
                    tx, ty,
                    tx, ty+th,
                    tx+tw, ty,
                    
                    tx+tw, ty+th,
                    tx, ty+th,
                    tx+tw, ty
                };
                
                r32 norms[18] = { 0 };
                
                calculate_heightmap_normal(verts, norms);
                calculate_heightmap_normal(verts+9, norms+9);
                
                if(k) {
                    foreach(l, 18) {
                        norms[l] *= -1;
                    }
                }
                
                foreach(i, sizeof(verts)/sizeof(verts[0])) {
                    da_push(vertices, verts[i]);
                }
                foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                    da_push(uvs, uvs_[i]);
                }
                foreach(i, sizeof(norms)/sizeof(norms[0])) {
                    da_push(normals, norms[i]);
                }
            }
            
            if(x && !(dungeon_tile_data[d->tiles[x-1][z]].flags & PIT)) {
                foreach(height, 4) {
                    
                    r32 verts[] = {
                        v00.x, v00.y-4+height, v00.z,
                        v10.x, v10.y-4+height, v10.z,
                        v00.x, v00.y-4+height+1, v00.z,
                        
                        v10.x, v10.y-4+height+1, v10.z,
                        v00.x, v00.y-4+height+1, v00.z,
                        v10.x, v10.y-4+height, v10.z
                    };
                    
                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,
                        
                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };
                    
                    r32 norms[18] = { 0 };
                    
                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);
                    
                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(x<MAP_W-1 && !(dungeon_tile_data[d->tiles[x+1][z]].flags & PIT)) {
                foreach(height, 4) {
                    
                    r32 verts[] = {
                        v01.x, v01.y-4+height, v01.z,
                        v11.x, v11.y-4+height, v11.z,
                        v01.x, v01.y-4+height+1, v01.z,
                        
                        v11.x, v11.y-4+height+1, v11.z,
                        v01.x, v01.y-4+height+1, v01.z,
                        v11.x, v11.y-4+height, v11.z
                    };
                    
                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,
                        
                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };
                    
                    r32 norms[18] = { 0 };
                    
                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);
                    
                    foreach(i, 18) {
                        norms[i] *= -1;
                    }
                    
                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(z && !(dungeon_tile_data[d->tiles[x][z-1]].flags & PIT)) {
                foreach(height, 4) {
                    r32 verts[] = {
                        v00.x, v00.y-4+height, v00.z,
                        v01.x, v01.y-4+height, v01.z,
                        v00.x, v00.y-4+height+1, v00.z,
                        
                        v01.x, v01.y-4+height+1, v01.z,
                        v00.x, v00.y-4+height+1, v00.z,
                        v01.x, v01.y-4+height, v01.z
                    };
                    
                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,
                        
                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };
                    
                    r32 norms[18] = { 0 };
                    
                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);
                    
                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(z < MAP_H-1 && !(dungeon_tile_data[d->tiles[x][z+1]].flags & PIT)) {
                foreach(height, 4) {
                    r32 verts[] = {
                        v10.x, v10.y-4+height, v10.z,
                        v11.x, v11.y-4+height, v11.z,
                        v10.x, v10.y-4+height+1, v10.z,
                        
                        v11.x, v11.y-4+height+1, v11.z,
                        v10.x, v10.y-4+height+1, v10.z,
                        v11.x, v11.y-4+height, v11.z
                    };
                    
                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,
                        
                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };
                    
                    r32 norms[18] = { 0 };
                    
                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);
                    
                    foreach(i, 18) {
                        norms[i] *= -1;
                    }
                    
                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
        }
        else {
            foreach(k, 2) {
                r32 verts[] = {
                    v00.x, v00.y + k*3, v00.z,
                    v01.x, v01.y + k*3, v01.z,
                    v10.x, v10.y + k*3, v10.z,
                    
                    v11.x, v11.y + k*3, v11.z,
                    v01.x, v01.y + k*3, v01.z,
                    v10.x, v10.y + k*3, v10.z,
                };
                
                if(k) {
                    tx = 64.f / textures[TEX_tile_dungeon].w,
                    ty = 0.f;
                }
                
                r32 uvs_[] = {
                    tx, ty,
                    tx, ty+th,
                    tx+tw, ty,
                    
                    tx+tw, ty+th,
                    tx, ty+th,
                    tx+tw, ty
                };
                
                r32 norms[18] = { 0 };
                
                calculate_heightmap_normal(verts, norms);
                calculate_heightmap_normal(verts+9, norms+9);
                
                if(k) {
                    foreach(l, 18) {
                        norms[l] *= -1;
                    }
                }
                
                foreach(i, sizeof(verts)/sizeof(verts[0])) {
                    da_push(vertices, verts[i]);
                }
                foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                    da_push(uvs, uvs_[i]);
                }
                foreach(i, sizeof(norms)/sizeof(norms[0])) {
                    da_push(normals, norms[i]);
                }
            }
        }
        
        if(++x >= MAP_W-1) {
            x = 0;
            if(++z >= MAP_H-1) {
                break;
            }
        }
    }
    
    glGenVertexArrays(1, &d->vao);
    glBindVertexArray(d->vao);
    
    glGenBuffers(1, &d->vertex_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, d->vertex_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(vertices), vertices, GL_STATIC_DRAW);
    
    glGenBuffers(1, &d->uv_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, d->uv_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(uvs), uvs, GL_STATIC_DRAW);
    
    glGenBuffers(1, &d->normal_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, d->normal_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(normals), normals, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    d->vertex_component_count = da_size(vertices);
    da_free(vertices);
    da_free(uvs);
    da_free(normals);
    
    d->render = init_g_buffer(window_w, window_h);
    init_light_state(&d->lighting);
}

void clean_up_dungeon_map(DungeonMap *d) {
    clean_up_g_buffer(&d->render);
    clean_up_particle_master(&d->particles);
    
    glDeleteBuffers(1, &d->normal_vbo);
    glDeleteBuffers(1, &d->uv_vbo);
    glDeleteBuffers(1, &d->vertex_vbo);
    glDeleteVertexArrays(1, &d->vao);
}

void collide_boxes_with_tiles(DungeonMap *d, BoxComponent *b, i32 count) {
    i32 current_box = 0;
    while(current_box < count) {
        v2 original_vel = b->vel,
        resolution_vel = v2(0, 0);
        
        for(r32 i = 1; i >= 0.2; i -= 0.2) {
            v2 pos1 = b->pos + i*original_vel*delta_t;
            
            forrng(x, pos1.x - b->size.x/2, pos1.x + b->size.x/2+1) {
                forrng(y, pos1.y - b->size.y/2, pos1.y + b->size.y/2+1) {
                    if(x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) {
                        if(dungeon_tile_data[d->tiles[x][y]].flags & WALL ||
                           dungeon_tile_data[d->tiles[x][y]].flags & PIT || 
                           dungeon_tile_data[d->tiles[x][y]].flags & DOOR) {
                            v2 overlap = v2(100000000, 10000000);
                            i8 overlap_x = 0, overlap_y = 0;
                            
                            if(pos1.x > x+0.5) {
                                if(x < MAP_W-1 &&
                                   !(dungeon_tile_data[d->tiles[x+1][y]].flags & WALL) &&
                                   !(dungeon_tile_data[d->tiles[x+1][y]].flags & PIT) &&
                                   !(dungeon_tile_data[d->tiles[x+1][y]].flags & DOOR)) {
                                    overlap.x = (x+1) - (pos1.x-b->size.x/2);
                                    overlap_x = 1;
                                }
                            }
                            else {
                                if(x &&
                                   !(dungeon_tile_data[d->tiles[x-1][y]].flags & WALL) &&
                                   !(dungeon_tile_data[d->tiles[x-1][y]].flags & PIT) &&
                                   !(dungeon_tile_data[d->tiles[x-1][y]].flags & DOOR)) {
                                    overlap.x = x-(pos1.x + b->size.x/2);
                                    overlap_x = 1;
                                }
                            }
                            
                            if(pos1.y > y+0.5) {
                                if(y < MAP_H-1 &&
                                   !(dungeon_tile_data[d->tiles[x][y+1]].flags & WALL) &&
                                   !(dungeon_tile_data[d->tiles[x][y+1]].flags & PIT) &&
                                   !(dungeon_tile_data[d->tiles[x][y+1]].flags & DOOR)) {
                                    overlap.y = (y+1) - (pos1.y-b->size.y/2);
                                    overlap_y = 1;
                                }
                            }
                            else {
                                if(y &&
                                   !(dungeon_tile_data[d->tiles[x][y-1]].flags & WALL) &&
                                   !(dungeon_tile_data[d->tiles[x][y-1]].flags & PIT) &&
                                   !(dungeon_tile_data[d->tiles[x][y-1]].flags & DOOR)) {
                                    overlap.y = y-(pos1.y + b->size.y/2);
                                    overlap_y = 1;
                                }
                            }
                            
                            v2 res = v2(0, 0);
                            if(fabs(overlap.x) < fabs(overlap.y)) {
                                if(overlap_x) {
                                    res = v2(overlap.x, 0);
                                }
                            }
                            else {
                                if(overlap_y) {
                                    res = v2(0, overlap.y);
                                }
                            }
                            
                            if(fabs((res*i).x) >= fabs(resolution_vel.x)) {
                                resolution_vel.x = (res*i).x;
                            }
                            if(fabs((res*i).y) >= fabs(resolution_vel.y)) {
                                resolution_vel.y = (res*i).y;
                            }
                        }
                    }
                }
            }
        }
        b->vel += resolution_vel/delta_t;
        
        ++b;
        ++current_box;
    }
}

void collide_boxes_with_projectiles(DungeonMap *d, i32 *id, BoxComponent *b, HealthComponent *h, DebuffComponent *debuff, i32 count) {
    foreach(i, count) {
        for(i32 j = 0; j < d->projectiles.count;) {
            if(d->projectiles.update[j].origin != id[i]) {
                v2 proj_pos0 = d->projectiles.update[j].pos;
                v2 proj_vel = d->projectiles.update[j].vel;
                i8 proj_hit = 0;
                
                for(r32 k = 0.2; k <= 1.0; k += 0.1) {
                    v2 proj_pos = proj_pos0 + proj_vel * k * delta_t;
                    // NOTE(Ryan): Check if the projectile is inside this box
                    if(proj_pos.x >= b[i].pos.x - b[i].size.x/2 &&
                       proj_pos.x <= b[i].pos.x + b[i].size.x/2 &&
                       proj_pos.y >= b[i].pos.y - b[i].size.y/2 &&
                       proj_pos.y <= b[i].pos.y + b[i].size.y/2) {
                        
                        r32 health_to_take = d->projectiles.update[j].strength * 
                            projectile_data[d->projectiles.type[j]].strength;
                        
                        r32 knockback_magnitude =2 * d->projectiles.update[j].strength * projectile_data[d->projectiles.type[j]].knockback;
                        
                        
                        // @On-hit effects
                        switch(projectile_data[d->projectiles.type[j]].effect_type) {
                            case EFFECT_fire: {
                                if(random32(0, 1) < d->projectiles.update[j].strength) {
                                    fire_debuff(debuff+i);
                                }
                                break;
                            }
                            case EFFECT_poison: {
                                if(random32(0, 1) < d->projectiles.update[j].strength) {
                                    poison_debuff(debuff+i);
                                }
                                break;
                            }
                            case EFFECT_aoe: {
                                break;
                            }
                            case EFFECT_slow: {
                                if(random32(0, 1) < d->projectiles.update[j].strength) {
                                    slow_debuff(debuff+i);
                                }
                                break;
                            }
                            case EFFECT_knockback: {
                                // NOTE(Ryan): If a random number in [0, 1] is < 0.3, 
                                //             add some knockback
                                if(random32(0, 1) < d->projectiles.update[j].strength) {
                                    knockback_magnitude *= 1.8;
                                }
                                break;
                            }
                            default: break;
                        }
                        
                        // subtract health
                        h[i].val -= health_to_take;
                        h[i].target = h[i].val;
                        
                        // add knockback
                        b[i].vel += (proj_vel) * knockback_magnitude;
                        
                        proj_hit = 1;
                        
                        break;
                    }
                }
                if(proj_hit) {
                    remove_projectile(d, j);
                }
                else {
                    ++j;
                }
            }
            else {
                ++j;
            }
        }
    }
}

void do_light(DungeonMap *d, v3 pos, v3 color, r32 radius, r32 intensity) {
    if(d->lighting.light_count < MAX_LIGHT) {
        Light l = { pos, color, radius, intensity };
        d->lighting.lights[d->lighting.light_count++] = l;
    }
}

void update_attacks(DungeonMap *d, i32 *id, AttackComponent *a, i32 count) {
    foreach(i, count) {
        if(a[i].attacking) {
            a[i].transition += (1-a[i].transition) * 3 * delta_t;
            a[i].charge += (a[i].mana-a[i].charge) * delta_t;
        }
        else {
            if(a[i].charge > 0.005) {
                a[i].mana -= a[i].charge;
                
                // @Attack Firing
                add_projectile(d, id[i], attack_data[a[i].type].projectile_type,
                               a[i].pos,
                               a[i].target,
                               a[i].charge);
                play_sound_at_point(&sounds[SOUND_spell_fire], 1, random32(0.8, 1.2), 0, AUDIO_entity, v3(a[i].pos.x, 0, a[i].pos.y));
            }
            a[i].mana += (1-a[i].mana) * delta_t * 0.25;
            a[i].transition -= a[i].transition * 2 * delta_t;
            a[i].charge = 0;
        }
    }
}

void update_ai(i32 *type, AIComponent *a, AttackComponent *attack, DebuffComponent *debuff, DungeonMap *d, Player *p, i32 count) {
    v2 target_pos;
    foreach(i, count) {
        if(!a->target_id) {
            target_pos = p ? p->box.pos+(p->box.vel*delta_t) : v2(-10000, -10000);
        }
        else {
            foreach(j, d->enemies.count) {
                if(d->enemies.id[j] == a->target_id) {
                    target_pos = d->enemies.box[j].pos;
                    break;
                }
            }
        }
        
        switch(a->state) {
            case AI_STATE_idle:
            case AI_STATE_roam: {
                attack->attacking = 0;
                r32 distance_to_target_2 = distance2_32(target_pos, a->pos);
                if(a->attack_type >= 0 && distance2_32(target_pos, a->pos) <= 64.f) {
                    r32 distance_to_target = sqrtf(distance_to_target_2);
                    v2 ai_to_target = target_pos - a->pos;
                    v2 ray_to_target = a->pos;
                    i8 blocked_by_wall = 0;
                    for(r32 j = 0.5f; j <= distance_to_target; j += 0.5f) {
                        ray_to_target += (ai_to_target / distance_to_target)*0.5f;
                        if((int)ray_to_target.x >= 0 && (int)ray_to_target.y < MAP_W &&
                           (int)ray_to_target.y >= 0 && (int)ray_to_target.y < MAP_H) {
                            if(dungeon_tile_data[d->tiles[(int)ray_to_target.x][(int)ray_to_target.y]].flags & WALL) {
                                blocked_by_wall = 1;
                                break;
                            }
                        }
                    }
                    if(!blocked_by_wall) {
                        a->state = AI_STATE_attack;
                    }
                }
                
                if(a->state == AI_STATE_roam) {
                    if(current_time >= a->wait_start_time + a->wait_duration) {
                        a->wait_start_time = current_time;
                        a->wait_duration = a->moving ? random32(3, 6) : random32(1, 5);
                        a->move_vel = a->moving ? v2(0, 0) : v2(random32(-5, 5), random32(-5, 5));
                        a->moving = !a->moving;
                    }
                }
                
                break;
            }
            case AI_STATE_attack: {
                r32 distance_to_target_2 = distance2_32(target_pos, a->pos);
                if(distance_to_target_2 >= 64.f) {
                    a->state = AI_STATE_roam;
                }
                else {
                    r32 distance_to_target = sqrtf(distance_to_target_2);
                    v2 ai_to_target = target_pos - a->pos;
                    v2 ray_to_target = a->pos;
                    i8 blocked_by_wall = 0;
                    for(r32 j = 0.5f; j <= distance_to_target; j += 0.5f) {
                        ray_to_target += (ai_to_target / distance_to_target)*0.5f;
                        if((int)ray_to_target.x >= 0 && (int)ray_to_target.y < MAP_W &&
                           (int)ray_to_target.y >= 0 && (int)ray_to_target.y < MAP_H) {
                            if(dungeon_tile_data[d->tiles[(int)ray_to_target.x][(int)ray_to_target.y]].flags & WALL) {
                                blocked_by_wall = 1;
                                break;
                            }
                        }
                    }
                    if(blocked_by_wall) {
                        a->state = AI_STATE_roam;
                    }
                }
                a->move_vel = (2*(target_pos - a->pos) / length(target_pos - a->pos)) * enemy_data[type[i]].speed;
                a->moving = 1;
                attack->target = ((target_pos - a->pos) / length(target_pos - a->pos))*16;
                if(attack->mana > 0.3) {
                    attack->attacking = 1;
                }
                attack->pos = a->pos;
                
                if(attack->charge > attack->mana*0.5) {
                    if(random32(0, attack->mana) < attack->charge/6) {
                        attack->attacking = 0;
                    }
                }
                
                break;
            }
            default: break;
        }
        
        if(debuff) {
            a->move_vel *= debuff->velocity_modifier;
        }
        
        ++a;
        ++attack;
    }
}

void track_sprites_to_boxes(DungeonMap *d, SpriteComponent *s, BoxComponent *b, i32 count) {
    foreach(i, count) {
        s->pos = v3(b->pos.x, dungeon_coordinate_height(d, b->pos.x, b->pos.y) + (s->th / 16.f) * 0.5, b->pos.y);
        ++s;
        ++b;
    }
}

void update_debuffs(DebuffComponent *d, u64 count, DungeonMap *map) {
    foreach(i, count) {
        v3 pos_3d = v3(d->pos.x, 0, d->pos.y);
        pos_3d.y = dungeon_coordinate_height(map, pos_3d.x, pos_3d.z) + 0.5;
        
        switch(d->damage_over_time_modifier) {
            case DAMAGE_OVER_TIME_fire: {
                do_particle(map, PARTICLE_fire, pos_3d, 
                            v3(random32(-0.1, 0.1), random32(0.5, 1.f), random32(-0.1, 0.1)),
                            random32(0.2, 0.6));
                break;
            }
            case DAMAGE_OVER_TIME_poison: {
                do_particle(map, PARTICLE_dark, pos_3d,
                            v3(random32(-0.5, 0.5), random32(0.5, 1.f), random32(-0.5, 0.5)),
                            random32(0.2, 0.6));
                break;
            }
            default: break;
        }
        
        if(d->velocity_modifier < 1) {
            do_particle(map, PARTICLE_ice, pos_3d + v3(random32(-0.5, 0.5), random32(-0.5, 0.5), random32(-0.5, 0.5)),
                        v3(random32(-0.1, 0.1), random32(-0.1, 0.1), random32(-0.1, 0.1)),
                        random32(0.2, 0.6));
        }
        
        if(d->velocity_modifier < 1 && 
           current_time >= d->velocity_start_time + d->velocity_seconds) {
            d->velocity_modifier = 1;
        }
        if(d->damage_over_time_modifier &&
           current_time >= d->damage_over_time_start_time + d->damage_over_time_seconds) {
            d->damage_over_time_modifier = 0;
        }
        ++d;
    }
}

void update_dungeon_map(DungeonMap *d, Player *p) {
    { // @Update dungeon projectiles
        for(u32 i = 0; i < (u32)d->projectiles.count;) {
            v2 last_pos = d->projectiles.update[i].pos;
            d->projectiles.update[i].pos += d->projectiles.update[i].vel*delta_t;
            d->projectiles.update[i].distance_traveled_sq += distance2_32(last_pos, d->projectiles.update[i].pos);
            
            do_light(d,
                     v3(d->projectiles.update[i].pos.x,
                        dungeon_coordinate_height(d, d->projectiles.update[i].pos.x, d->projectiles.update[i].pos.y) + 0.5,
                        d->projectiles.update[i].pos.y),
                     projectile_data[d->projectiles.type[i]].color, 10 + 10*d->projectiles.update[i].strength, 2 + 2*d->projectiles.update[i].strength);
            
            if(current_time >= d->projectiles.update[i].particle_start_time + 1/60.f &&
               projectile_data[d->projectiles.type[i]].particle_type >= 0) {
                foreach(j, 1) {
                    do_particle(d, projectile_data[d->projectiles.type[i]].particle_type,
                                v3(d->projectiles.update[i].pos.x,
                                   dungeon_coordinate_height(d, d->projectiles.update[i].pos.x, d->projectiles.update[i].pos.y) + 0.5,
                                   d->projectiles.update[i].pos.y),
                                v3(random32(-0.25, 0.25), random32(-0.25, 0.25), random32(-0.25, 0.25)) * d->projectiles.update[i].strength,
                                random32(0.01, 0.25 + 0.75*d->projectiles.update[i].strength));
                }
                d->projectiles.update[i].particle_start_time = current_time;
            }
            
            i32 tile_x = d->projectiles.update[i].pos.x,
            tile_z = d->projectiles.update[i].pos.y;
            
            if(tile_x < 0 || tile_x >= MAP_W || tile_z < 0 || tile_z >= MAP_H ||
               dungeon_tile_data[d->tiles[tile_x][tile_z]].flags & WALL ||
               (d->projectiles.update[i].distance_traveled_sq >= d->projectiles.update[i].range_sq &&
                d->projectiles.update[i].range_sq > 0.f)) {
                remove_projectile(d, i);
            }
            else {
                ++i;
            }
        }
    }
    
    if(p) { // @Update dungeon player
        i32 player_id = 0;
        
        collide_boxes_with_tiles(d, &p->box, 1);
        collide_boxes_with_projectiles(d, &player_id, &p->box, &p->health, &p->debuff, 1);
        update_boxes(&p->box, 1);
        
        { // collide with collectibles
            v4 bb1 = v4(p->box.pos.x - p->box.size.x/2,
                        p->box.pos.y - p->box.size.y/2,
                        p->box.pos.x + p->box.size.x/2,
                        p->box.pos.y + p->box.size.y/2);
            
            v4 bb2;
            
            for(u32 i = 0; i < d->collectibles.count;) {
                bb2 = v4(d->collectibles.box[i].pos.x - d->collectibles.box[i].size.x/2,
                         d->collectibles.box[i].pos.y - d->collectibles.box[i].size.y/2,
                         d->collectibles.box[i].pos.x + d->collectibles.box[i].size.x/2,
                         d->collectibles.box[i].pos.y + d->collectibles.box[i].size.y/2);
                if(bb1.x <= bb2.z && bb1.z >= bb2.x &&
                   bb1.y <= bb2.w && bb1.w >= bb2.y) {
                    i8 collected = 0;
                    foreach(j, 3) {
                        if(p->inventory[j] < 0) {
                            collected = 1;
                            p->inventory[j] = d->collectibles.type[i];
                            break;
                        }
                    }
                    if(collected) {
                        if(d->collectibles.type[i] == COLLECTIBLE_key) {
                            play_sound(&sounds[SOUND_key_get], 1, 1, 0, AUDIO_entity);
                        }
                        remove_collectible(d, i);
                    }
                    else {
                        ++i;
                    }
                }
                else {
                    ++i;
                }
            }
        }
        
        { // collide with doors
            foreach(i, 3) {
                if(p->inventory[i] == COLLECTIBLE_key) {
                    r32 player_x = p->box.pos.x - p->box.size.x/2;
                    r32 player_y = p->box.pos.y - p->box.size.y/2;
                    r32 player_w = p->box.size.x;
                    r32 player_h = p->box.size.y;
                    foreach(j, d->doors.count) {
                        if(player_x + player_w >= d->doors.x[j] && 
                           player_x <= d->doors.x[j] + 1 && 
                           player_y + player_h >= d->doors.y[j] && 
                           player_y <= d->doors.y[j] + 1) {
                            p->inventory[i] = -1;
                            d->tiles[d->doors.x[j]][d->doors.y[j]] = DUNGEON_TILE_broken_stone;
                            remove_door(&d->doors, j);
                            play_sound(&sounds[SOUND_door], 1, 1, 0, AUDIO_entity);
                            break;
                        }
                    }
                    
                    break;
                }
            }
        }
        
        { // collide with ladders
            r32 player_x = p->box.pos.x - p->box.size.x/2;
            r32 player_y = p->box.pos.y - p->box.size.y/2;
            r32 player_w = p->box.size.x;
            r32 player_h = p->box.size.y;
            foreach(j, d->ladders.count) {
                if(player_x + player_w >= d->ladders.x[j] && 
                   player_x <= d->ladders.x[j] + 1 && 
                   player_y + player_h >= d->ladders.y[j] && 
                   player_y <= d->ladders.y[j] + 1) {
                    next_state = init_dungeon_state();
                }
            }
        }
        
        p->attack.pos = p->debuff.pos = p->box.pos;
        
        update_debuffs(&p->debuff, 1, d);
        update_attacks(d, &player_id, &p->attack, 1);
        update_health(&p->health, &p->debuff, 1);
    }
    
    { // @Update dungeon enemies
        move_boxes_with_ai(d->enemies.box, d->enemies.ai, d->enemies.count);
        
        collide_boxes_with_tiles(d, d->enemies.box, d->enemies.count);
        collide_boxes_with_projectiles(d, d->enemies.id, d->enemies.box, d->enemies.health, d->enemies.debuff, d->enemies.count);
        update_boxes(d->enemies.box, d->enemies.count);
        
        // update sprite position
        track_sprites_to_boxes(d, d->enemies.sprite, d->enemies.box, d->enemies.count);
        
        // update_debuffs
        track_debuffs_to_boxes(d->enemies.debuff, d->enemies.box, d->enemies.count);
        update_debuffs(d->enemies.debuff, d->enemies.count, d);
        
        // update attacks
        foreach(i, d->enemies.count) {
            d->enemies.attack[i].pos = d->enemies.box[i].pos;
        }
        
        update_attacks(d, d->enemies.id, d->enemies.attack, d->enemies.count);
        
        // update ai position
        foreach(i, d->enemies.count) {
            d->enemies.ai[i].pos = d->enemies.box[i].pos;
        }
        update_ai(d->enemies.type, d->enemies.ai, d->enemies.attack, d->enemies.debuff, d, p, d->enemies.count);
        
        // update sprite animation TY
        foreach(i, d->enemies.count) {
            switch(d->enemies.ai[i].state) {
                case AI_STATE_idle:
                case AI_STATE_roam: {
                    d->enemies.anim_wait[i] -= delta_t;
                    if(d->enemies.anim_wait[i] <= 0.f) {
                        d->enemies.sprite[i].ty = !d->enemies.sprite[i].ty ? 24 : 0;
                        d->enemies.anim_wait[i] = 1.f;
                    }
                    break;
                }
                case AI_STATE_attack: {
                    if(d->enemies.attack[i].attacking) {
                        d->enemies.sprite[i].ty = 48;
                        d->enemies.anim_wait[i] = 0.2f;
                    }
                    else {
                        if(d->enemies.anim_wait[i] >= 0.f) {
                            d->enemies.sprite[i].ty = 72;
                        }
                        else {
                            d->enemies.sprite[i].ty = 0;
                        }
                    }
                    default: break;
                }
            }
        }
        
        // update enemy health
        update_health(d->enemies.health, d->enemies.debuff, d->enemies.count);
        for(u32 i = 0; i < d->enemies.count;) {
            if(d->enemies.health[i].val <= 0.f) {
                if(random32(0,1) >= .5f){
                    add_collectible(d, random32(0, 2), d->enemies.box[i].pos);
                }
                //do_particle(d, PARTICLE_death, d->enemies.box[i].pos, 1, 1);
                remove_enemy(d, d->enemies.id[i]);
            }
            else {
                ++i;
            }
        }
    }
    
    { // @Update dungeon collectibles
        collide_boxes_with_tiles(d, d->collectibles.box, d->collectibles.count);
        update_boxes(d->collectibles.box, d->collectibles.count);
        track_sprites_to_boxes(d, d->collectibles.sprite, d->collectibles.box, d->collectibles.count);
    }
    
    { // @Update dungeon particles
        update_particle_master(&d->particles);
    }
    
    { // @Update dungeon lighting
        i8 light_count = 0;
        
        set_shader(SHADER_world_render);
        {
            {
                light_count = 0;
                
                char str_1[24] = "lights[x].pos",
                str_2[24] = "lights[xx].pos";
                
                foreach(i, d->lighting.light_count) {
                    if(light_count < 10) {
                        str_1[7] = '0' + light_count;
                        uniform3f(uniform_loc(str_1), d->lighting.lights[i].pos);
                    }
                    else {
                        str_2[7] = '0' + light_count/10;
                        str_2[8] = '0' + (light_count - 10*(light_count/10));
                        uniform3f(uniform_loc(str_2), d->lighting.lights[i].pos);
                    }
                    
                    ++light_count;
                }
            }
            
            {
                light_count = 0;
                
                char str_1[24] = "lights[x].color",
                str_2[24] = "lights[xx].color";
                
                foreach(i, d->lighting.light_count) {
                    if(light_count < 10) {
                        str_1[7] = '0' + light_count;
                        uniform3f(uniform_loc(str_1), d->lighting.lights[i].color);
                    }
                    else {
                        str_2[7] = '0' + light_count/10;
                        str_2[8] = '0' + (light_count - 10*(light_count/10));
                        uniform3f(uniform_loc(str_2), d->lighting.lights[i].color);
                    }
                    
                    ++light_count;
                }
            }
            
            {
                light_count = 0;
                
                char str_1[24] = "lights[x].radius",
                str_2[24] = "lights[xx].radius";
                
                foreach(i, d->lighting.light_count) {
                    if(light_count < 10) {
                        str_1[7] = '0' + light_count;
                        uniform1f(uniform_loc(str_1), d->lighting.lights[i].radius);
                    }
                    else {
                        str_2[7] = '0' + light_count/10;
                        str_2[8] = '0' + (light_count - 10*(light_count/10));
                        uniform1f(uniform_loc(str_2), d->lighting.lights[i].radius);
                    }
                    
                    ++light_count;
                }
            }
            
            {
                light_count = 0;
                
                char str_1[24] = "lights[x].intensity",
                str_2[24] = "lights[xx].intensity";
                
                foreach(i, d->lighting.light_count) {
                    if(light_count < 10) {
                        str_1[7] = '0' + light_count;
                        uniform1f(uniform_loc(str_1), d->lighting.lights[i].intensity);
                    }
                    else {
                        str_2[7] = '0' + light_count/10;
                        str_2[8] = '0' + (light_count - 10*(light_count/10));
                        uniform1f(uniform_loc(str_2), d->lighting.lights[i].intensity);
                    }
                    
                    ++light_count;
                }
            }
        }
        
        uniform1f(uniform_loc("brightness"), 0.2);
        uniform3f(uniform_loc("light_vector"), d->light_vector);
        uniform1i(uniform_loc("light_count"), light_count);
        set_shader(-1);
        
        d->lighting.light_count = 0;
    }
}

void draw_dungeon_map_begin(DungeonMap *d) {
    force_g_buffer_size(&d->render, window_w, window_h);
    
    clear_g_buffer(&d->render);
    bind_g_buffer(&d->render);
    
    { // @Draw dungeon heightmap/terrain
        model = m4d(1);
        
        set_shader(SHADER_heightmap);
        
        glBindTexture(GL_TEXTURE_2D, textures[TEX_tile_dungeon].id);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);
        
        glUniform3f(glGetUniformLocation(active_shader, "light_vector"), d->light_vector.x, d->light_vector.y, d->light_vector.z);
        
        glBindVertexArray(d->vao);
        
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, d->vertex_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, d->uv_vbo);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, d->normal_vbo);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glDrawArrays(GL_TRIANGLES, 0, d->vertex_component_count);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);
        
        glBindVertexArray(0);
        
        set_shader(0);
    }
    
    // @Draw doors
    draw_sprite_components(d->doors.sprite, d->doors.count);
    
    // @Draw ladders
    draw_sprite_components(d->ladders.sprite, d->ladders.count);
    
    // @Draw enemies
    draw_sprite_components(d->enemies.sprite, d->enemies.count);
    
    // @Draw collectibles
    draw_sprite_components(d->collectibles.sprite, d->collectibles.count);
}

void draw_dungeon_map_end(DungeonMap *d) {
    bind_g_buffer(0);
    
    m4 last_view = view;
    
    set_shader(SHADER_world_render);
    uniform_m4(uniform_loc("projection3d"), projection);
    uniform_m4(uniform_loc("view3d"), view);
    prepare_for_ui_render();
    glDepthMask(GL_FALSE);
    draw_ui_g_buffer(&d->render, v4(0, 0, window_w, window_h));
    glDepthMask(GL_TRUE);
    set_shader(-1);
    
    copy_g_buffer_depth(&d->render);
    
    // @Draw particles
    prepare_for_world_render();
    view = last_view;
    draw_particle_master(&d->particles);
}
