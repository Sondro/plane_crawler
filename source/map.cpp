#include "component.cpp"
#include "player.cpp"
#include "enemy.cpp"
#include "particle.cpp"
#include "projectile.cpp"

#define WALL    0x01
#define PIT     0x02

enum {
    TILE_BRICK,
    TILE_MOSSY_BRICK,
    TILE_BRICK_WALL,
    TILE_MOSSY_BRICK_WALL,
    TILE_DIRT,
    TILE_WATER,
    TILE_PIT,
    MAX_TILE
};

global
struct {
    i8 tx, ty,
       flags;
} tile_data[MAX_TILE] = {
    { 0, 0, 0 },
    { 1, 0, 0 },
    { 0, 0, WALL }, 
    { 1, 0, WALL },
    { 2, 0, 0 },
    { 1, 1, 0 },
    { 1, 0, PIT },
};

struct Map {
    i8 tiles[MAP_W][MAP_H];
    r32 heights[MAP_W+1][MAP_H+1];
    
    Player      player;
    EnemySet    enemies;    
    
    ProjectileSet projectiles;
    ParticleMaster particles;

    v3 light_vector;

    u64 vertex_component_count;
    GLuint vao,
           vertex_vbo,
           uv_vbo,
           normal_vbo;
};

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

r32 map_coordinate_height(Map *m, r32 x, r32 z) {
    if(x >= 0 && x < MAP_W-1 &&
       z >= 0 && z < MAP_H-1) {
        int height_x = (int)x, 
            height_z = (int)z;

        v3 q00, q10,
           q01, q11;

        q00 = v3(0, m->heights[height_x][height_z], 0);
        q10 = v3(1, m->heights[height_x+1][height_z], 0);
        q01 = v3(0, m->heights[height_x][height_z+1], 1);
        q11 = v3(1, m->heights[height_x+1][height_z+1], 1);

        v3 p = v3(x - height_x, 0, z - height_z);
        
        r32 side_0 = q00.y + p.z*(q01.y - q00.y),
            side_1 = q10.y + p.z*(q11.y - q10.y);

        p.y = side_0 + p.x*(side_1 - side_0);

        return p.y;
    }
    return 0.f;
}

void do_particle(Map *m, i8 type, v3 pos, v3 vel, r32 scale) {
    ParticleSet *s = m->particles.sets + type;
    if(s->count < MAX_PARTICLE_COUNT) {
        r32 particle_data[PARTICLE_DATA_LENGTH] = {
            pos.x,
            pos.y,
            pos.z,
            vel.x,
            vel.y,
            vel.z,
            1,
            scale
        };
        memcpy(s->particle_data + PARTICLE_DATA_LENGTH*s->count, particle_data, PARTICLE_DATA_LENGTH*sizeof(r32));
        ++s->count;
    }
}

void add_enemy(Map *m, i16 type, v2 pos) {
    if(m->enemies.count < MAX_ENEMY_COUNT) {
        u32 i = m->enemies.count;

        i32 max_id = 1;
        foreach(i, m->enemies.count) {
            if(m->enemies.id[i] > max_id) {
                max_id = m->enemies.id[i]+1;
            }
        }
        
        m->enemies.id[i] = max_id;
        init_enemy(type, pos, 
                   m->enemies.box + i,
                   m->enemies.sprite + i,
                   m->enemies.health + i,
                   m->enemies.attack + i);
        
        ++m->enemies.count;
    }
}

void remove_enemy(Map *m, i32 id) {
    foreach(i, m->enemies.count) {
        if(m->enemies.id[i] == id) {
            memmove(m->enemies.id + i, m->enemies.id + i+1, sizeof(i32) * (m->enemies.count - i - 1));
            memmove(m->enemies.box + i, m->enemies.box + i+1, sizeof(BoxComponent) * (m->enemies.count - i - 1));
            memmove(m->enemies.sprite + i, m->enemies.sprite + i+1, sizeof(SpriteComponent) * (m->enemies.count - i - 1));
            memmove(m->enemies.health + i, m->enemies.health + i+1, sizeof(HealthComponent) * (m->enemies.count - i - 1));
            memmove(m->enemies.attack + i, m->enemies.attack + i+1, sizeof(AttackComponent) * (m->enemies.count - i - 1));
            --m->enemies.count;
            break;
        }
    }
}

void add_projectile(Map *m, i32 origin, i16 type, v2 pos, v2 vel, r32 strength) {
    if(m->projectiles.count < MAX_PROJECTILE_COUNT) {
        m->projectiles.type[m->projectiles.count] = type;
        m->projectiles.update[m->projectiles.count].origin = origin;
        m->projectiles.update[m->projectiles.count].pos = pos;
        m->projectiles.update[m->projectiles.count].vel = vel;
        m->projectiles.update[m->projectiles.count].strength = strength;
        ++m->projectiles.count;
    }
}

void remove_projectile(Map *m, i32 i) {
    if(i >= 0 && i < m->projectiles.count) {
        foreach(j, 100) {
            r32 pitch = random32(0, PI/2),
                yaw = random32(0, 2*PI);
            do_particle(m, 
                        projectile_data[m->projectiles.type[i]].particle_type,
                        v3(m->projectiles.update[i].pos.x, 
                           map_coordinate_height(m, m->projectiles.update[i].pos.x, m->projectiles.update[i].pos.y) + 0.5,
                           m->projectiles.update[i].pos.y),
                        v3(cos(yaw), sin(pitch), sin(yaw)) / random32(20, 40),
                        random32(0.05, 0.15 + m->projectiles.update[i].strength*0.25));
        }
        
        memmove(m->projectiles.type+i, m->projectiles.type+i+1, sizeof(i16) * (m->projectiles.count - i - 1));
        memmove(m->projectiles.update+i, m->projectiles.update+i+1, sizeof(ProjectileUpdate) * (m->projectiles.count - i - 1));
        --m->projectiles.count; 
    }
}

void generate_map(Map *m) {
    m->enemies.count = 0;
    m->projectiles.count = 0;
    init_particle_master(&m->particles);
    m->light_vector = v3(1, 1, 1) / sqrt(3);
    
    foreach(i, MAP_W)
    foreach(j, MAP_H) {
        m->tiles[i][j] = TILE_BRICK_WALL;
        m->heights[i][j] = 0;
    }
    
    // @Room generation
    i32 room_count = 8;

    struct {
        int x, y, w, h;
    } rooms[room_count];

    foreach(i, room_count) {
        rooms[i].x = random32(0, MAP_W-1);
        rooms[i].y = random32(0, MAP_H-1);
        rooms[i].w = random32(4, 24);
        rooms[i].h = random32(4, 24);
    }

    foreach(i, room_count) {
        forrng(x, rooms[i].x, rooms[i].x + rooms[i].w+1)
        forrng(y, rooms[i].y, rooms[i].y + rooms[i].h+1) {
            if(x >= 1 && x < MAP_W-1 && y >= 1 && y < MAP_H-1) {
                if(x < rooms[i].x + rooms[i].w && x < MAP_W-2 && 
                   y < rooms[i].y + rooms[i].h && y < MAP_H-2) {
                    m->tiles[x][y] = TILE_DIRT;
                }
                m->heights[x][y] = perlin_2d(x, y, 0.15, 12)*1.1;
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
        v3 v00 = v3(x,   m->heights[x][z],     z),
           v01 = v3(x+1, m->heights[x+1][z],   z),
           v10 = v3(x,   m->heights[x][z+1],   z+1),
           v11 = v3(x+1, m->heights[x+1][z+1], z+1);
        
         r32 tx = (tile_data[m->tiles[x][z]].tx*16.f)/(r32)textures[TEX_TILE].w,
             ty = (tile_data[m->tiles[x][z]].ty*16.f)/(r32)textures[TEX_TILE].h,
             tw = 16.f/textures[TEX_TILE].w,
             th = 16.f/textures[TEX_TILE].h;
        

        if(tile_data[m->tiles[x][z]].flags & WALL) {
            if(x && !(tile_data[m->tiles[x-1][z]].flags & WALL)) {
                foreach(height, 4) {
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
            if(x<MAP_W-1 && !(tile_data[m->tiles[x+1][z]].flags & WALL)) {
                foreach(height, 4) {
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
            if(z && !(tile_data[m->tiles[x][z-1]].flags & WALL)) {
                foreach(height, 4) {
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
            if(z < MAP_H-1 && !(tile_data[m->tiles[x][z+1]].flags & WALL)) {
                foreach(height, 4) {
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
        else if(tile_data[m->tiles[x][z]].flags & PIT) {
            if(x && !(tile_data[m->tiles[x-1][z]].flags & PIT)) {
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
            if(x<MAP_W-1 && !(tile_data[m->tiles[x+1][z]].flags & PIT)) {
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
            if(z && !(tile_data[m->tiles[x][z-1]].flags & PIT)) {
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
            if(z < MAP_H-1 && !(tile_data[m->tiles[x][z+1]].flags & PIT)) {
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
            r32 verts[] = {
                v00.x, v00.y, v00.z,
                v01.x, v01.y, v01.z,
                v10.x, v10.y, v10.z,
                
                v11.x, v11.y, v11.z,
                v01.x, v01.y, v01.z,
                v10.x, v10.y, v10.z,
            }; 
             
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

        if(++x >= MAP_W-1) {
            x = 0;
            if(++z >= MAP_H-1) {
                break;
            }
        }
    }

    glGenVertexArrays(1, &m->vao);
    glBindVertexArray(m->vao);

    glGenBuffers(1, &m->vertex_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m->vertex_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(vertices), vertices, GL_STATIC_DRAW);
    
    glGenBuffers(1, &m->uv_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m->uv_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(uvs), uvs, GL_STATIC_DRAW);

    glGenBuffers(1, &m->normal_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m->normal_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(normals), normals, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    m->vertex_component_count = da_size(vertices);
    da_free(vertices);
    da_free(uvs);
    da_free(normals);
    
    i8 spawned_player = 0;

    foreach(i, MAP_W)
    foreach(j, MAP_H) {
        if(!(tile_data[m->tiles[i][j]].flags & WALL) &&
           !(tile_data[m->tiles[i][j]].flags & PIT)) {
            if(!spawned_player) {
                m->player = init_player(v2(i+0.5, j+0.5));
                spawned_player = 1;
            }

            if(random32(0, 1) < 0.01) {
                add_enemy(m, random32(0, MAX_ENEMY - 0.0001), v2(i, j));
            }
        }
    }
}

void clean_up_map(Map *m) {
    clean_up_particle_master(&m->particles);
    
    glDeleteBuffers(1, &m->normal_vbo);
    glDeleteBuffers(1, &m->uv_vbo);
    glDeleteBuffers(1, &m->vertex_vbo);
    glDeleteVertexArrays(1, &m->vao); 
}

void collide_boxes_with_tiles(Map *m, BoxComponent *b, i32 count) {
    i32 current_box = 0;
    while(current_box < count) {
        v2 original_vel = b->vel,
           resolution_vel = v2(0, 0);
        i8 found_colliding_tiles = 0,
           resolved_x = 0,
           resolved_y = 0;

        for(r32 i = 1; i >= 0.2; i -= 0.2) {
            v2 pos1 = b->pos + i*original_vel;

            forrng(x, pos1.x - b->size.x/2, pos1.x + b->size.x/2+1) {
                forrng(y, pos1.y - b->size.y/2, pos1.y + b->size.y/2+1) {
                    if(x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) {
                        if(tile_data[m->tiles[x][y]].flags & WALL ||
                           tile_data[m->tiles[x][y]].flags & PIT) {
                            v2 overlap = v2(100000000, 10000000);
                            i8 overlap_x = 0, overlap_y = 0;

                            if(pos1.x > x+0.5) {
                                if(x < MAP_W-1 &&
                                   !(tile_data[m->tiles[x+1][y]].flags & WALL) &&
                                   !(tile_data[m->tiles[x+1][y]].flags & PIT)) {
                                    overlap.x = (x+1) - (pos1.x-b->size.x/2);
                                    overlap_x = 1;
                                }
                            }
                            else {
                                if(x &&
                                   !(tile_data[m->tiles[x-1][y]].flags & WALL) &&
                                   !(tile_data[m->tiles[x-1][y]].flags & PIT)) {
                                    overlap.x = x-(pos1.x + b->size.x/2);
                                    overlap_x = 1;
                                }
                            }
                            
                            if(pos1.y > y+0.5) {
                                if(y < MAP_H-1 &&
                                   !(tile_data[m->tiles[x][y+1]].flags & WALL) &&
                                   !(tile_data[m->tiles[x][y+1]].flags & PIT)) {
                                    overlap.y = (y+1) - (pos1.y-b->size.y/2);
                                    overlap_y = 1;
                                }
                            }
                            else {
                                if(y &&
                                   !(tile_data[m->tiles[x][y-1]].flags & WALL) &&
                                   !(tile_data[m->tiles[x][y-1]].flags & PIT)) {
                                    overlap.y = y-(pos1.y + b->size.y/2);
                                    overlap_y = 1;
                                }
                            }
                                  
                            v2 resolution = v2(0, 0);
                            if(fabs(overlap.x) < fabs(overlap.y)) {
                                if(!resolved_x && overlap_x) {
                                    resolution = v2(overlap.x, 0);
                                    resolved_x = 1;
                                }
                            } 
                            else {
                                if(!resolved_y && overlap_y) {
                                    resolution = v2(0, overlap.y);
                                    resolved_y = 1;
                                }
                            }

                            resolution_vel += resolution*i;
                            
                            found_colliding_tiles = 1;
                        }
                    }
                }
            }

            if(found_colliding_tiles) {
                b->vel += resolution_vel;
                break;
            }
        }
        ++b;
        ++current_box;
    }
}

void collide_boxes_with_projectiles(Map *m, i32 *id, BoxComponent *b, HealthComponent *h, i32 count) { 
    foreach(i, count) {
        foreach(j, m->projectiles.count) {
            if(m->projectiles.update[j].origin != id[i]) {
                v2 proj_pos0 = m->projectiles.update[j].pos,
                   proj_vel = m->projectiles.update[j].vel;

                for(r32 k = 0.2; k <= 1.0; k += 0.2) {
                    v2 proj_pos = proj_pos0 + proj_vel * k;
                    if(proj_pos.x >= b[i].pos.x - b[i].size.x/2 &&
                       proj_pos.x <= b[i].pos.x + b[i].size.x/2 &&
                       proj_pos.y >= b[i].pos.y - b[i].size.y/2 &&
                       proj_pos.y <= b[i].pos.y + b[i].size.y/2) {
                        h[i].target -= m->projectiles.update[j].strength/2;
                        b[i].vel += proj_vel / 4;
                        remove_projectile(m, j);
                        break;
                    }
                }
            }
        }
    }
}

void update_attacks(Map *m, i32 *id, AttackComponent *a, i32 count) {
    foreach(i, count) {
        if(a[i].attacking) {
            a[i].transition += (1-a[i].transition)*0.2;
            a[i].charge += (1-a[i].charge) * 0.05;
        }
        else {
            if(a[i].charge > 0.005) {
                // @Attack Firing
                switch(a[i].type) {
                    case ATTACK_FIREBALL: {
                        add_projectile(m, id[i], PROJECTILE_FIRE, 
                                       a[i].pos,
                                       a[i].target,
                                       a[i].charge);
                        break;
                    }
                    default: break;
                }
            }
            a[i].mana += (1-a[i].mana) * 0.005;
            a[i].transition *= 0.9;
            a[i].charge = 0;
        }
    }
}

void update_map(Map *m) {
    { // @Update projectiles
        for(u32 i = 0; i < (u32)m->projectiles.count;) {
            m->projectiles.update[i].pos += m->projectiles.update[i].vel;
            
            foreach(j, 1) {
                do_particle(m, projectile_data[m->projectiles.type[i]].particle_type,
                            v3(m->projectiles.update[i].pos.x,
                               map_coordinate_height(m, m->projectiles.update[i].pos.x, m->projectiles.update[i].pos.y) + 0.5,
                               m->projectiles.update[i].pos.y),
                            v3(random32(-0.025, 0.025), random32(-0.025, 0.025), random32(-0.025, 0.025)) * m->projectiles.update[i].strength,
                            random32(0.01, 0.25 + 0.75*m->projectiles.update[i].strength));
            }
            
            i32 tile_x = m->projectiles.update[i].pos.x,
                tile_z = m->projectiles.update[i].pos.y; 

            if(tile_x < 0 || tile_x >= MAP_W || tile_z < 0 || tile_z >= MAP_H ||
               tile_data[m->tiles[tile_x][tile_z]].flags & WALL) {
                remove_projectile(m, i);                
            }
            else {
                ++i;
            }
        }
    }
    
    { // @Update player
        i32 player_id = 0;

        collide_boxes_with_tiles(m, &m->player.box, 1);
        collide_boxes_with_projectiles(m, &player_id, &m->player.box, &m->player.health, 1);
        m->player.box.pos += m->player.box.vel;
        m->player.attack.pos = m->player.box.pos;
        update_attacks(m, &player_id, &m->player.attack, 1);
        m->player.health.val += (m->player.health.target - m->player.health.val) * 0.1;
    }

    { // @Update enemies
        collide_boxes_with_tiles(m, m->enemies.box, m->enemies.count);
        collide_boxes_with_projectiles(m, m->enemies.id, m->enemies.box, m->enemies.health, m->enemies.count);

        // update sprite position
        foreach(i, m->enemies.count) {
            m->enemies.box[i].pos += m->enemies.box[i].vel;
            m->enemies.sprite[i].pos = v3(m->enemies.box[i].pos.x, 
                                          map_coordinate_height(m, m->enemies.box[i].pos.x, m->enemies.box[i].pos.y) + 0.5, 
                                          m->enemies.box[i].pos.y);
        } 

        // update attack position
        foreach(i, m->enemies.count) {
            m->enemies.attack[i].pos = m->enemies.box[i].pos;
        }

        update_attacks(m, m->enemies.id, m->enemies.attack, m->enemies.count);
        
        // update enemy health
        for(u32 i = 0; i < m->enemies.count;) {
            m->enemies.health[i].val += (m->enemies.health[i].target - m->enemies.health[i].val) * 0.1;
            if(m->enemies.health[i].val <= 0.f) {
                remove_enemy(m, m->enemies.id[i]);
            }
            else {
                ++i;
            }
        }
    }

    { // @Update particles
        update_particle_master(&m->particles);
    } 
}

void draw_map(Map *m) {
    { // @Draw heightmap/terrain
        reset_model();

        set_shader(SHADER_HEIGHTMAP);
        
        glBindTexture(GL_TEXTURE_2D, textures[TEX_TILE].id);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);
        
        glUniform3f(glGetUniformLocation(active_shader, "light_vector"), m->light_vector.x, m->light_vector.y, m->light_vector.z);

        glBindVertexArray(m->vao);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, m->vertex_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, m->uv_vbo);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, m->normal_vbo);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glDrawArrays(GL_TRIANGLES, 0, m->vertex_component_count);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);

        set_shader(0);
    }

    { // @Draw enemies
        draw_sprite_components(m->enemies.sprite, m->enemies.count); 
    }

    // @Draw particles
    draw_particle_master(&m->particles);
}
