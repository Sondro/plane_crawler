#include "player.cpp"
#include "enemy.cpp"

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
    { 0, 1, 0 },
};

struct Map {
    i8 tiles[MAP_W][MAP_H];
    r32 heights[MAP_W+1][MAP_H+1];
     
    i16 enemy_count;
    Enemy enemies[MAX_ENEMY_COUNT];

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
    m->enemy_count = 0;
    
    foreach(i, MAP_W+1) {
        foreach(j, MAP_H+1) {
            m->heights[i][j] = 2*perlin_2d(i, j, 0.1, 12);
        }   
    }

    foreach(i, MAP_W) {
        foreach(j, MAP_H) {
            m->tiles[i][j] = TILE_DIRT;
        }
    }

    forrng(i, 32, 64) {
        m->tiles[i][32] = TILE_BRICK_WALL;
    }
   
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
}

void clean_up_map(Map *m) {
    glDeleteBuffers(1, &m->normal_vbo);
    glDeleteBuffers(1, &m->uv_vbo);
    glDeleteBuffers(1, &m->vertex_vbo);
    glDeleteVertexArrays(1, &m->vao); 
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

void update_map(Map *m) {
    { // @Update enemies
        
    }
}

void draw_map(Map *m) {
    {
        reset_model();

        set_shader(SHADER_HEIGHTMAP);
        
        glBindTexture(GL_TEXTURE_2D, textures[TEX_TILE].id);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);

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
}
