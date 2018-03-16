#define ALIGN_LEFT   0
#define ALIGN_RIGHT  1
#define ALIGN_CENTER 2

#define reset_model()        { model = HMM_Mat4d(1.f); }
#define translate(x, y, z)   { model = HMM_Multiply(model, HMM_Translate(HMM_Vec3(x, y, z))); }
#define scale(x, y, z)       { model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(x, y, z))); }
#define rotate(a, x, y, z)   { model = HMM_Multiply(model, HMM_Rotate(a, HMM_Vec3(x, y, z))); }
#define look_at(p, t)        { view = HMM_LookAt(p, t, HMM_Vec3(0, 1, 0)); }

global m4 model, view, projection,
          view_inv, projection_inv;

global v2 uv_offset,
          uv_range;

global Texture small_font,
               tiles;

global Shader rect_shader,
              texture_quad_shader,
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

    rect_shader = load_shader("rect");
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
            0, 1,
            0, 0,
            1, 1,
            1, 0
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

    clean_up_shader(&rect_shader);
    clean_up_shader(&texture_quad_shader);
    clean_up_shader(&heightmap_shader);

    clean_up_texture(&small_font);
    clean_up_texture(&tiles);
}

void prepare_for_world_render() {
    projection = HMM_Perspective(FIELD_OF_VIEW, (r32)window_w/window_h, 1.f, 1000.f);
    reset_model();
    view = HMM_Mat4d(1);
}

void prepare_for_ui_render() {
    glClear(GL_DEPTH_BUFFER_BIT);

    projection = HMM_Perspective(FIELD_OF_VIEW, (r32)window_w/window_h, 0.01f, 10.f);
    reset_model();
    look_at(v3(0, 0, 1), v3(0, 0, 0));

    projection_inv = m4_inverse(projection);
    view_inv = m4_inverse(view);
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
    glBindTexture(GL_TEXTURE_2D, t ? t->id : 0);
    uv_offset = v2((r32)tx/t->w, (r32)ty/t->h);
    uv_range = v2((r32)tw/t->w, (r32)th/t->h);
    glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), uv_offset.x, uv_offset.y);
    glUniform2f(glGetUniformLocation(active_shader, "uv_range"), uv_range.x, uv_range.y);
}

//
// @Note (Ryan)
//
// Expects all matrix transformations and shader
// calls to be made before-hand. Calls all of the
// necessary OpenGL stuff to draw a quad. It's
// here for when more rotation/shader control is
// needed... usually the standard draw_ui_rect or
// draw_ui_tex functions will do just fine for most
// cases.
//

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

void draw_ui_rect(v4 color, v4 bb, r32 thickness) {
    set_shader(&rect_shader);
    {
        v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;

        glUniform4f(glGetUniformLocation(active_shader, "rect_color"), color.x, color.y, color.z, color.w);
        glUniform2f(glGetUniformLocation(active_shader, "thickness"), thickness/bb.z, thickness/bb.w);

        reset_model();
        translate(pos.x, pos.y, 0);
        scale(size.x, size.y, 1);
        draw_quad();
    }
    set_shader(0);
}

void draw_ui_filled_rect(v4 color, v4 bb, r32 thickness) {
    set_shader(&rect_shader);
    {
        v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;

        glUniform4f(glGetUniformLocation(active_shader, "rect_color"), color.x, color.y, color.z, color.w);
        glUniform2f(glGetUniformLocation(active_shader, "thickness"), -1, -1);

        reset_model();
        translate(pos.x, pos.y, 0);
        scale(size.x, size.y, 1);
        draw_quad();
    }
    set_shader(0);
}

void draw_ui_text(const char *text, int align, v2 position) {
    set_shader(&texture_quad_shader);
    {
        v4 pos = v4((position.x/window_w)*2 - 1, -(position.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(24.f/window_w, 32.f/window_h, 0, 0);
        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;

        u32 text_len = strlen(text);

        reset_model();
        translate(pos.x, pos.y, 0);

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
            if(c >= 65 && c <= 90) {
                tx = 6*(c-65);
                ty = 0;
            }
            else if(c >= 48 && c <= 57) {
                tx = 6*(c-48);
                ty = 8;
            }
            else {
                switch(c) {
                    case '.':  { tx = 60; ty = 8; break; }
                    case ',':  { tx = 66; ty = 8; break; }
                    case ';':  { tx = 72; ty = 8; break; }
                    case ':':  { tx = 78; ty = 8; break; }
                    case '(':  { tx = 84; ty = 8; break; }
                    case ')':  { tx = 90; ty = 8; break; }
                    case '\'': { tx = 96; ty = 8; break; }
                    case '"':  { tx = 102; ty = 8; break; }
                    case '!':  { tx = 108; ty = 8; break; }
                    case '?':  { tx = 114; ty = 8; break; }
                    case '-':  { tx = 120; ty = 8; break; }
                    case '+':  { tx = 126; ty = 8; break; }
                    case '*':  { tx = 132; ty = 8; break; }
                    case '/':  { tx = 138; ty = 8; break; }
                    case '\\': { tx = 144; ty = 8; break; }
                    default: break;
                }
            }

            if(tx >= 0 && ty >= 0) {
                uv_offset = v2((r32)tx/small_font.w, (r32)ty/small_font.h);
                glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), uv_offset.x, uv_offset.y);

                {
                    m4 char_model = HMM_Multiply(model, HMM_Scale(v3(size.x, size.y, 1)));
                    glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &char_model.Elements[0][0]);
                }

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            translate(size.x*2, 0, 0);

            tx = -1;
            ty = -1;
        }

        bind_texture(0);

        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);
    }
    set_shader(0);
}
