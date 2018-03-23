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
        
    EnemySet enemies;
    i8 enemy_anim_timer;
    ParticleMaster particles;
    ProjectileSet projectiles;

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

void generate_map(Map *m) {
    m->enemies.count = 0;
    m->enemy_anim_timer = 60;
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
    
    foreach(i, MAP_W)
    foreach(j, MAP_H) {
        if(!(tile_data[m->tiles[i][j]].flags & WALL) &&
           !(tile_data[m->tiles[i][j]].flags & PIT)) {
            if(random32(0, 1) < 0.05) {
                m->enemies.type[m->enemies.count] = ENEMY_JELLY;
                m->enemies.update[m->enemies.count].pos = v2(i+0.5, j+0.5);
                m->enemies.update[m->enemies.count].vel = v2(0, 0);
                m->enemies.update[m->enemies.count].update_dir_t = 0;
                ++m->enemies.count; 
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

void do_projectile(Map *m, i16 type, v2 pos, v2 vel) {
    if(m->projectiles.count < MAX_PROJECTILE_COUNT) {
        m->projectiles.type[m->projectiles.count] = type;
        m->projectiles.pos_vel[m->projectiles.count] = v4(pos.x, pos.y, vel.x, vel.y);
        ++m->projectiles.count;
    }
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

void collide_entity(Map *m, v2 *pos0, v2 *vel, r32 size) {
    v2 original_vel = *vel,
       resolution_vel = v2(0, 0);
    i8 found_colliding_tiles = 0,
       resolved_x = 0,
       resolved_y = 0;

    for(r32 i = 1; i >= 0.2; i -= 0.2) {
        v2 pos1 = *pos0 + i*original_vel;

        forrng(x, pos1.x-size/2, pos1.x+size/2+1) {
            forrng(y, pos1.y-size/2, pos1.y+size/2+1) {
                if(x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) {
                    if(tile_data[m->tiles[x][y]].flags & WALL ||
                       tile_data[m->tiles[x][y]].flags & PIT) {
                        v2 overlap = v2(100000000, 10000000);
                        i8 overlap_x = 0, overlap_y = 0;

                        if(pos1.x > x+0.5) {
                            if(x < MAP_W-1 &&
                               !(tile_data[m->tiles[x+1][y]].flags & WALL) &&
                               !(tile_data[m->tiles[x+1][y]].flags & PIT)) {
                                overlap.x = (x+1) - (pos1.x-size/2);
                                overlap_x = 1;
                            }
                        }
                        else {
                            if(x &&
                               !(tile_data[m->tiles[x-1][y]].flags & WALL) &&
                               !(tile_data[m->tiles[x-1][y]].flags & PIT)) {
                                overlap.x = x-(pos1.x + size/2);
                                overlap_x = 1;
                            }
                        }
                        
                        if(pos1.y > y+0.5) {
                            if(y < MAP_H-1 &&
                               !(tile_data[m->tiles[x][y+1]].flags & WALL) &&
                               !(tile_data[m->tiles[x][y+1]].flags & PIT)) {
                                overlap.y = (y+1) - (pos1.y-size/2);
                                overlap_y = 1;
                            }
                        }
                        else {
                            if(y &&
                               !(tile_data[m->tiles[x][y-1]].flags & WALL) &&
                               !(tile_data[m->tiles[x][y-1]].flags & PIT)) {
                                overlap.y = y-(pos1.y + size/2);
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
            *vel += resolution_vel;
            break;
        }
    }
}

void update_map(Map *m) {
    if(!--m->enemy_anim_timer) {
        m->enemy_anim_timer = 120;
    }
    
    { // @Update projectiles
        for(u32 i = 0; i < (u32)m->projectiles.count;) {
            m->projectiles.pos_vel[i].XY += m->projectiles.pos_vel[i].ZW;
            do_particle(m, projectile_data[m->projectiles.type[i]].particle_type,
                        v3(m->projectiles.pos_vel[i].x,
                           map_coordinate_height(m, m->projectiles.pos_vel[i].x, m->projectiles.pos_vel[i].y) + 0.5,
                           m->projectiles.pos_vel[i].y),
                        v3(0, 0.001, 0), random32(0.01, 0.25));
            i32 tile_x = m->projectiles.pos_vel[i].x,
                tile_z = m->projectiles.pos_vel[i].y; 

            if(tile_x < 0 || tile_x >= MAP_W || tile_z < 0 || tile_z >= MAP_H ||
               tile_data[m->tiles[tile_x][tile_z]].flags & WALL ||
               tile_data[m->tiles[tile_x][tile_z]].flags & PIT) {
                foreach(j, 100) {
                    r32 pitch = random32(0, PI/2),
                        yaw = random32(0, 2*PI);
                    do_particle(m, 
                                projectile_data[m->projectiles.type[i]].particle_type,
                                v3(m->projectiles.pos_vel[i].x, 
                                   map_coordinate_height(m, m->projectiles.pos_vel[i].x, m->projectiles.pos_vel[i].y) + 0.5,
                                   m->projectiles.pos_vel[i].y),
                                v3(cos(yaw), sin(pitch), sin(yaw)) / random32(20, 40),
                                random32(0.05, 0.15));
                }
                
                memmove(m->projectiles.type+i, m->projectiles.type+i+1, sizeof(i16) * (m->projectiles.count - i - 1));
                memmove(m->projectiles.pos_vel+i, m->projectiles.pos_vel+i+1, sizeof(v4) * (m->projectiles.count - i - 1));
                --m->projectiles.count;
            }
            else {
                ++i;
            }
        }
    }

    { // @Update enemies
        foreach(i, m->enemies.count) {
            collide_entity(m, &m->enemies.update[i].pos, &m->enemies.update[i].vel, 0.25);
            m->enemies.update[i].pos += m->enemies.update[i].vel;
            
            m->enemies.update[i].update_dir_t -= 0.005;
            if(m->enemies.update[i].update_dir_t < 0.005) {
                m->enemies.update[i].vel = v2(random32(-0.01, 0.01), random32(-0.01, 0.01));
                m->enemies.update[i].update_dir_t = random32(1, 5);
            }
            else if(m->enemies.update[i].update_dir_t < 0.3) {
                m->enemies.update[i].vel *= 0.85;
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
        v2 pos;
        foreach(i, m->enemies.count) {
            pos = m->enemies.update[i].pos;
            draw_billboard_texture(textures+TEX_ENEMY, 
                                   v4(enemy_data[m->enemies.type[i]].tx*16, (m->enemy_anim_timer < 30)*16, 16, 16), 
                                   v3(pos.X, map_coordinate_height(m, pos.X, pos.Y)+0.5, pos.Y), 
                                   v2(0.5, 0.5));
        }
    }

    // @Draw particles
    draw_particle_master(&m->particles);
}
