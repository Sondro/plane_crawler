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
    { 0, 0, 0 }, 
    { 1, 0, 0 },
    { 2, 0, 0 },
    { 1, 1, 0 },
    { 0, 1, 0 },
};

struct Map {
    i8 tiles[MAP_W][MAP_H];
    r32 heights[MAP_W+1][MAP_H+1];
     
    i16 enemy_count;
    Enemy enemies[MAX_ENEMY_COUNT];

    GLuint vao,
           vertex_vbo,
           uv_vbo,
           normal_vbo;
};

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

    r32 *vertices = heap_alloc(r32, (MAP_W) * (MAP_H) * 2 * 9),
        *uvs = vertices,
        *normals = vertices;

    glGenVertexArrays(1, &m->vao);
    glBindVertexArray(m->vao);

    { // @Vertex Position Generation
        u32 write_x = 0,
            write_z = 0;

        for(u32 i = 0; i < (MAP_W*MAP_H) * 2 * 9; i += 18) {
            vertices[i]     = write_x;
            vertices[i+1]   = m->heights[write_x][write_z];
            vertices[i+2]   = write_z;

            vertices[i+3]   = write_x;
            vertices[i+4]   = m->heights[write_x][write_z+1];
            vertices[i+5]   = write_z+1;

            vertices[i+6]   = write_x+1;
            vertices[i+7]   = m->heights[write_x+1][write_z];
            vertices[i+8]   = write_z;

            vertices[i+9]   = write_x+1;
            vertices[i+10]  = m->heights[write_x+1][write_z];
            vertices[i+11]  = write_z;

            vertices[i+12]  = write_x;
            vertices[i+13]  = m->heights[write_x][write_z+1];
            vertices[i+14]  = write_z+1;

            vertices[i+15]  = write_x+1;
            vertices[i+16]  = m->heights[write_x+1][write_z+1];
            vertices[i+17]  = write_z+1;

            if(++write_x >= MAP_W) {
                write_x = 0;
                ++write_z;
            }
        }

        glGenBuffers(1, &m->vertex_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m->vertex_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * (MAP_W*MAP_H) * 9 * 2, vertices, GL_STATIC_DRAW);
    }
    
    { // @Vertex UV Generation
        u32 write_x = 0,
            write_z = 0;
        
        r32 uv_offset_x = 0,
            uv_offset_y = 0,
            uv_range_x = 16.f/textures[TEX_TILE].w,
            uv_range_y = 16.f/textures[TEX_TILE].h;
        
        for(u32 i = 0; i < (MAP_W*MAP_H) * 2 * 6; i += 12) {
            uv_offset_x = (tile_data[m->tiles[write_x][write_z]].tx*16) / (r32)textures[TEX_TILE].w;
            uv_offset_y = (tile_data[m->tiles[write_x][write_z]].ty*16) / (r32)textures[TEX_TILE].h;

            uvs[i]     = uv_offset_x;
            uvs[i+1]   = uv_offset_y;

            uvs[i+2]   = uv_offset_x;
            uvs[i+3]   = uv_offset_y + uv_range_y;

            uvs[i+4]   = uv_offset_x + uv_range_x;
            uvs[i+5]   = uv_offset_y;

            uvs[i+6]   = uv_offset_x + uv_range_x;
            uvs[i+7]   = uv_offset_y;

            uvs[i+8]   = uv_offset_x;
            uvs[i+9]   = uv_offset_y + uv_range_y;

            uvs[i+10]  = uv_offset_x + uv_range_x;
            uvs[i+11]  = uv_offset_y + uv_range_y;

            if(++write_x >= MAP_W) {
                write_x = 0;
                ++write_z;
            }
        }

        glGenBuffers(1, &m->uv_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m->uv_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * (MAP_W*MAP_H) * 6 * 2, uvs, GL_STATIC_DRAW);
    }

    { // @Vertex Normal Generation
        u32 write_x = 0,
            write_z = 0;

        for(u32 i = 0; i < (MAP_W*MAP_H) * 2 * 9; i += 18) {
            hmm_v3 v1 = HMM_Vec3(
                        1,
                        m->heights[write_x+1][write_z] - m->heights[write_x][write_z],
                        0
                    ),

                   v2 = HMM_Vec3(
                        0,
                        m->heights[write_x][write_z+1] - m->heights[write_x][write_z],
                        1
                    );

            hmm_v3 normal = HMM_Cross(v1, v2);

            normal /= (HMM_Length(v1)*HMM_Length(v2));
            normal *= -1;

            normals[i]     = normal.X;
            normals[i+1]   = normal.Y;
            normals[i+2]   = normal.Z;

            normals[i+3]   = normal.X;
            normals[i+4]   = normal.Y;
            normals[i+5]   = normal.Z;

            normals[i+6]   = normal.X;
            normals[i+7]   = normal.Y;
            normals[i+8]   = normal.Z;

            v1 = HMM_Vec3(
                -1,
                m->heights[write_x+1][write_z] - m->heights[write_x+1][write_z+1],
                0
            ),

            v2 = HMM_Vec3(
                0,
                m->heights[write_x][write_z+1] - m->heights[write_x+1][write_z+1],
                -1
            );

            normal = HMM_Cross(v1, v2);

            normal /= (HMM_Length(v1)*HMM_Length(v2));
            normal *= -1;

            normals[i+9]   = normal.X;
            normals[i+10]  = normal.Y;
            normals[i+11]  = normal.Z;

            normals[i+12]  = normal.X;
            normals[i+13]  = normal.Y;
            normals[i+14]  = normal.Z;

            normals[i+15]  = normal.X;
            normals[i+16]  = normal.Y;
            normals[i+17]  = normal.Z;

            if(++write_x >= MAP_W) {
                write_x = 0;
                ++write_z;
            }
        }

        glGenBuffers(1, &m->normal_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m->normal_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * (MAP_W) * (MAP_H) * 9 * 2, normals, GL_STATIC_DRAW);
    }
    
    /*
    bind_fbo(&m->render_fbo);
    {
        glGenTextures(1, &m->depth_tex);
        glBindTexture(GL_TEXTURE_2D, m->depth_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, m->render_fbo.w, m->render_fbo.h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m->depth_tex, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    bind_fbo(0);
    */
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
        
        glDrawArrays(GL_TRIANGLES, 0, (MAP_W*MAP_H)*2*3);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);

        set_shader(0);
    }
}
