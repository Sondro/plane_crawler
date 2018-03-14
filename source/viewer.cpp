struct ViewerData {
    hmm_v3 eye, target;

    r32 heights[HEIGHTMAP_W][HEIGHTMAP_H],
        colors[HEIGHTMAP_W][HEIGHTMAP_H][3];

    GLuint vao,
           vertex_buffer,
           normal_buffer,
           color_buffer;

    Shader heightmap_shader;
};

void init_viewer() {
    memory = malloc(sizeof(ViewerData));
    ViewerData *v = (ViewerData *)memory;

    v->eye = HMM_Vec3(-10, 10, 0);
    v->target = HMM_Vec3(1, 1, 1);

    foreach(i, HEIGHTMAP_W) {
        foreach(j, HEIGHTMAP_H) { 
            r32 val = perlin_2d(i, j, 0.05, 8);
            
            v->heights[i][j] = val*16;
            foreach(k, 3) {
                v->colors[i][j][k] = 0.8;
            }
            /*
            v->heights[i][j] = random32(0, 1);
            v->colors[i][j][0] = random32(0, 1);
            v->colors[i][j][1] = random32(0, 1);
            v->colors[i][j][2] = random32(0, 1);
            */
        }
    }

    r32 *vertices = heap_alloc(r32, (HEIGHTMAP_W - 1) * (HEIGHTMAP_H - 1) * 2 * 9),
        *normals = vertices,
        *colors = vertices;

    glGenVertexArrays(1, &v->vao);
    glBindVertexArray(v->vao);

    { // @Vertex Position Generation
        u32 write_x = 0,
            write_z = 0;

        for(u32 i = 0; i < (HEIGHTMAP_W - 1) * (HEIGHTMAP_H - 1) * 2 * 9; i += 18) {
            vertices[i]     = (write_x)*HEIGHTMAP_CELL_SIZE;
            vertices[i+1]   = v->heights[write_x][write_z];
            vertices[i+2]   = (write_z)*HEIGHTMAP_CELL_SIZE;

            vertices[i+3]   = (write_x)*HEIGHTMAP_CELL_SIZE;
            vertices[i+4]   = v->heights[write_x][write_z+1];
            vertices[i+5]   = (write_z+1)*HEIGHTMAP_CELL_SIZE;

            vertices[i+6]   = (write_x+1)*HEIGHTMAP_CELL_SIZE;
            vertices[i+7]   = v->heights[write_x+1][write_z];
            vertices[i+8]   = (write_z)*HEIGHTMAP_CELL_SIZE;

            vertices[i+9]   = (write_x+1)*HEIGHTMAP_CELL_SIZE;
            vertices[i+10]  = v->heights[write_x+1][write_z];
            vertices[i+11]  = (write_z)*HEIGHTMAP_CELL_SIZE;

            vertices[i+12]  = (write_x)*HEIGHTMAP_CELL_SIZE;
            vertices[i+13]  = v->heights[write_x][write_z+1];
            vertices[i+14]  = (write_z+1)*HEIGHTMAP_CELL_SIZE;

            vertices[i+15]  = (write_x+1)*HEIGHTMAP_CELL_SIZE;
            vertices[i+16]  = v->heights[write_x+1][write_z+1];
            vertices[i+17]  = (write_z+1)*HEIGHTMAP_CELL_SIZE;

            if(++write_x >= HEIGHTMAP_W-1) {
                write_x = 0;
                ++write_z;
            }
        }

        glGenBuffers(1, &v->vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, v->vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * (HEIGHTMAP_W - 1) * (HEIGHTMAP_H - 1) * 9 * 2, vertices, GL_STATIC_DRAW);
    }

    { // @Vertex Normal Generation
        u32 write_x = 0,
            write_z = 0;

        for(u32 i = 0; i < (HEIGHTMAP_W - 1) * (HEIGHTMAP_H - 1) * 2 * 9; i += 18) {
            hmm_v3 v1 = HMM_Vec3(
                        HEIGHTMAP_CELL_SIZE,
                        v->heights[write_x+1][write_z] - v->heights[write_x][write_z],
                        0
                    ),

                   v2 = HMM_Vec3(
                        0,
                        v->heights[write_x][write_z+1] - v->heights[write_x][write_z],
                        HEIGHTMAP_CELL_SIZE
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
                -HEIGHTMAP_CELL_SIZE,
                v->heights[write_x+1][write_z] - v->heights[write_x+1][write_z+1],
                0
            ),

            v2 = HMM_Vec3(
                0,
                v->heights[write_x][write_z+1] - v->heights[write_x+1][write_z+1],
                -HEIGHTMAP_CELL_SIZE
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

            if(++write_x >= HEIGHTMAP_W-1) {
                write_x = 0;
                ++write_z;
            }
        }

        glGenBuffers(1, &v->normal_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, v->normal_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * (HEIGHTMAP_W - 1) * (HEIGHTMAP_H - 1) * 9 * 2, normals, GL_STATIC_DRAW);
    }

    { // @Vertex Color Generation
        u32 write_x = 0,
            write_z = 0;

        for(u32 i = 0; i < (HEIGHTMAP_W - 1) * (HEIGHTMAP_H - 1) * 2 * 9; i += 18) {
            colors[i]     = v->colors[write_x][write_z][0];
            colors[i+1]   = v->colors[write_x][write_z][1];
            colors[i+2]   = v->colors[write_x][write_z][2];

            colors[i+3]   = v->colors[write_x][write_z][0];
            colors[i+4]   = v->colors[write_x][write_z][1];
            colors[i+5]   = v->colors[write_x][write_z][2];

            colors[i+6]   = v->colors[write_x][write_z][0];
            colors[i+7]   = v->colors[write_x][write_z][1];
            colors[i+8]   = v->colors[write_x][write_z][2];

            colors[i+9]   = v->colors[write_x+1][write_z][0];
            colors[i+10]  = v->colors[write_x+1][write_z][1];
            colors[i+11]  = v->colors[write_x+1][write_z][2];

            colors[i+12]  = v->colors[write_x+1][write_z][0];
            colors[i+13]  = v->colors[write_x+1][write_z][1];
            colors[i+14]  = v->colors[write_x+1][write_z][2];

            colors[i+15]  = v->colors[write_x+1][write_z][0];
            colors[i+16]  = v->colors[write_x+1][write_z][1];
            colors[i+17]  = v->colors[write_x+1][write_z][2];

            if(++write_x >= HEIGHTMAP_W-1) {
                write_x = 0;
                ++write_z;
            }
        }

        glGenBuffers(1, &v->color_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, v->color_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * (HEIGHTMAP_W - 1) * (HEIGHTMAP_H - 1) * 9 * 2, colors, GL_STATIC_DRAW);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    free(vertices);

    v->heightmap_shader = load_shader("heightmap");
}

void clean_up_viewer() {
    ViewerData *v = (ViewerData *)memory;

    clean_up_shader(&v->heightmap_shader);

    glDeleteBuffers(1, &v->color_buffer);
    glDeleteBuffers(1, &v->normal_buffer);
    glDeleteBuffers(1, &v->vertex_buffer);
    glDeleteVertexArrays(1, &v->vao);

    free(memory);
    memory = 0;
}

void update_viewer() {
    ViewerData *v = (ViewerData *)memory;

    v->eye = HMM_Vec3(30*cos(current_time/2), 20, 30*sin(current_time/2));

    model = HMM_Mat4d(1.f);
    view = HMM_LookAt(v->eye, HMM_Vec3(0, 0, 0), HMM_Vec3(0.f, 1.f, 0.f));
    projection = HMM_Perspective(90.f, (r32)window_w/window_h, 0.1f, 100.f);

    { // draw heightmap
        model = HMM_Translate(HMM_Vec3(-HEIGHTMAP_W*HEIGHTMAP_CELL_SIZE/2, 0, -HEIGHTMAP_H*HEIGHTMAP_CELL_SIZE/2));

        GLuint active_shader = v->heightmap_shader.id;

        glUseProgram(active_shader);

        glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);

        glBindVertexArray(v->vao);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, v->vertex_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, v->normal_buffer);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, v->color_buffer);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glDrawArrays(GL_TRIANGLES, 0, (HEIGHTMAP_W-1)*(HEIGHTMAP_H-1)*2*3);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);

        glUseProgram(0);
    }
}
