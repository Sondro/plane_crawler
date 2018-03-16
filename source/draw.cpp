#define reset_model()        { model = HMM_Mat4d(1.f); }
#define translate(x, y, z)   { model = HMM_Multiply(model, HMM_Translate(HMM_Vec3(x, y, z))); }
#define scale(x, y, z)       { model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(x, y, z))); }
#define rotate(a, x, y, z)   { model = HMM_Multiply(model, HMM_Rotate(a, HMM_Vec3(x, y, z))); }
#define look_at(p, t   )     { view = HMM_LookAt(p, t, HMM_Vec3(0, 1, 0)); }

global m4 model, view, projection;

global v2 uv_offset,
          uv_range;

global Texture small_font,
               tiles;

global Shader texture_quad_shader,
              heightmap_shader;

GLuint active_shader = 0,

       quad_vao = 0,
       quad_vertex_vbo = 0,
       quad_uv_vbo = 0,
       quad_normal_vbo = 0;

void init_draw() {
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    small_font = load_texture("font");
    tiles = load_texture("tiles");
    texture_quad_shader = load_shader("texture_quad");
    heightmap_shader = load_shader("heightmap");

    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);
    {
        r32 quad_vertices[] = {
            -1, -1, 0,
            -1, 1, 0,
            1, -1, 0,
            1, 1, 0
        };

        r32 quad_uvs[] = {
            0, 0,
            0, 1,
            1, 0,
            1, 1
        };

        r32 quad_normals[] = {
            0, 0, 1,
            0, 0, 1,
            0, 0, 1,
            0, 0, 1
        };

        glGenBuffers(1, &quad_vertex_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

        glGenBuffers(1, &quad_uv_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, quad_uv_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * sizeof(quad_uvs), quad_uvs, GL_STATIC_DRAW);

        glGenBuffers(1, &quad_normal_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, quad_normal_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * sizeof(quad_normals), quad_normals, GL_STATIC_DRAW);
    }
    glBindVertexArray(0);
}

void clean_up_draw() {
    glDeleteBuffers(1, &quad_vertex_vbo);
    glDeleteBuffers(1, &quad_uv_vbo);
    glDeleteBuffers(1, &quad_normal_vbo);

    clean_up_shader(&texture_quad_shader);
    clean_up_shader(&heightmap_shader);

    clean_up_texture(&small_font);
    clean_up_texture(&tiles);
}

void set_shader(Shader *s) {
    active_shader = s ? s->id : 0;
    glUseProgram(active_shader);
}

void bind_texture(Texture *t) {
    glBindTexture(GL_TEXTURE_2D, t ? t->id : 0);
    uv_offset = v2(0, 0);
    uv_range = v2(1, 1);
    glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), uv_offset.x, uv_offset.y);
    glUniform2f(glGetUniformLocation(active_shader, "uv_range"), uv_range.x, uv_range.y);
}

void bind_texture(Texture *t, i32 tx, i32 ty, i32 tw, i32 th) {
    uv_offset = v2((r32)tx/t->w, (r32)ty/t->h);
    uv_range = v2((r32)tw/t->w, (r32)th/t->h);
    glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), uv_offset.x, uv_offset.y);
    glUniform2f(glGetUniformLocation(active_shader, "uv_range"), uv_range.x, uv_range.y);
}

void draw_quad() {
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);

    glBindVertexArray(quad_vao);
    {
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, quad_uv_vbo);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, quad_normal_vbo);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);
    }
    glBindVertexArray(0);
}

void draw_text(const char *text) {
    set_shader(&texture_quad_shader);
    
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);

    glBindVertexArray(quad_vao);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, quad_uv_vbo);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, quad_normal_vbo);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    char c;
    i32 tx = 0,
        ty = 0;

    bind_texture(&small_font, 0, 0, 6, 8);

    while((c = *text++)) {
        c = toupper(c);
        if(c >= 65 && c <= 91) {
            tx = 6*(c-65);
            ty = 0;
        }

        uv_offset = v2((r32)tx/small_font.w, (r32)ty/small_font.h); 
        glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), uv_offset.x, uv_offset.y);
        
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        translate(18, 0, 0);
    }

    bind_texture(0);

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindVertexArray(0);
    
    set_shader(0);
}
