#define WALL    0x01
#define PIT     0x02

enum {
    TILE_BRICK,
    TILE_BRICK_WALL,
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
    { 0, 0, 0 },
    { 0, 0, 0 }, 
    { 0, 0, 0 },
    { 0, 0, 0 },
};

struct Map {
    i8 tiles[MAP_W][MAP_H];
    r32 heights[MAP_W+1][MAP_H+1];

    GLuint vao,
           vertex_vbo,
           uv_vbo,
           normal_vbo;
};

void generate_map(Map *m) {
    foreach(i, MAP_W+1) {
        foreach(j, MAP_H+1) {
            m->heights[i][j] = 4*perlin_2d(i, j, 0.1, 12);
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

        for(u32 i = 0; i < (MAP_W*MAP_H) * 2 * 6; i += 12) {
            uvs[i]     = 0;
            uvs[i+1]   = 0;

            uvs[i+2]   = 0;
            uvs[i+3]   = 1;

            uvs[i+4]   = 1;
            uvs[i+5]   = 0;

            uvs[i+6]   = 1;
            uvs[i+7]   = 0;

            uvs[i+8]   = 0;
            uvs[i+9]   = 1;

            uvs[i+10]  = 1;
            uvs[i+11]  = 1;

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

}

void clean_up_map(Map *m) {
    glDeleteBuffers(1, &m->normal_vbo);
    glDeleteBuffers(1, &m->vertex_vbo);
    glDeleteVertexArrays(1, &m->vao); 
}

void draw_map(Map *m) {
    { // draw heightmap
        model = HMM_Translate(HMM_Vec3(-MAP_W/2, 0, -MAP_H/2));
 
        set_shader(&heightmap_shader);
        
        glBindTexture(GL_TEXTURE_2D, tiles.id);
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
