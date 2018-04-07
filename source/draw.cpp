#define ALIGN_LEFT   0
#define ALIGN_RIGHT  1
#define ALIGN_CENTER 2

#define uniform1i(location, v)  glUniform1i(location, (v))
#define uniform1f(location, v)  glUniform1f(location, (v))
#define uniform2f(location, v)  glUniform2f(location, (v).x, (v).y)
#define uniform3f(location, v)  glUniform3f(location, (v).x, (v).y, (v).z)
#define uniform4f(location, v)  glUniform4f(location, (v).x, (v).y, (v).z, (v).w)
#define uniform_m4(location, v) glUniformMatrix4fv(location, 1, GL_FALSE, &v.Elements[0][0])
#define uniform_loc(name)       glGetUniformLocation(active_shader, name)

#define enable_depth()  { glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE); }
#define disable_depth() { glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE); render_z = 0; }
#define set_render_z(z) { render_z = (z); }

struct FBO {
    GLuint id, texture;
    i32 w, h;
};

enum {
    GBUFFER_TEXTURE_ALBEDO,
    GBUFFER_TEXTURE_NORMAL,
    MAX_GBUFFER_TEXTURE
};

struct GBuffer {
    GLuint fbo,
           textures[MAX_GBUFFER_TEXTURE],
           depth_texture;
    i32 w, h;
};

global m4 model, view, projection,
          view_inv, projection_inv;

global v2 uv_offset,
          uv_range;

GLuint active_shader = 0,

       quad_vao = 0,
       quad_vertex_vbo = 0,
       quad_uv_vbo = 0,
       quad_normal_vbo = 0;

FBO *active_fbo = 0;

void init_draw() {
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

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

    foreach(i, MAX_SHADER) {
        clean_up_shader(shaders+i);
    }

    foreach(i, MAX_TEX) {
        clean_up_texture(textures+i);
    }
}

FBO init_fbo(i32 w, i32 h) {
    FBO f;
    f.w = w;
    f.h = h;

    glGenFramebuffers(1, &f.id);
    glBindFramebuffer(GL_FRAMEBUFFER, f.id);

    glGenTextures(1, &f.texture);
    glBindTexture(GL_TEXTURE_2D, f.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, f.texture, 0);
    GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return f;
}

void clean_up_fbo(FBO *f) {
    glDeleteTextures(1, &f->texture);
    f->texture = 0;
    glDeleteFramebuffers(1, &f->id);
    f->id = 0;
}

void force_fbo_size(FBO *f, i32 w, i32 h) {
    if(f->w != w || f->h != h) {
        clean_up_fbo(f);
        *f = init_fbo(w, h);
    }
}

GBuffer init_g_buffer(i32 w, i32 h) {
    GBuffer g;
    g.w = w;
    g.h = h;

    glGenFramebuffers(1, &g.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g.fbo);

    glGenTextures(MAX_GBUFFER_TEXTURE, g.textures);
    glGenTextures(1, &g.depth_texture);

    glBindTexture(GL_TEXTURE_2D, g.textures[GBUFFER_TEXTURE_ALBEDO]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g.textures[GBUFFER_TEXTURE_ALBEDO], 0);

    glBindTexture(GL_TEXTURE_2D, g.textures[GBUFFER_TEXTURE_NORMAL]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g.textures[GBUFFER_TEXTURE_NORMAL], 0);

    glBindTexture(GL_TEXTURE_2D, g.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g.depth_texture, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    return g;
}

void clean_up_g_buffer(GBuffer *g) {
    glDeleteTextures(1, &g->depth_texture);
    glDeleteTextures(MAX_GBUFFER_TEXTURE, g->textures);
    glDeleteFramebuffers(1, &g->fbo);
}

void force_g_buffer_size(GBuffer *g, i32 w, i32 h) {
    if(g->w != w || g->h != h) {
        clean_up_g_buffer(g);
        *g = init_g_buffer(w, h);
    }
}

void prepare_for_world_render() {
    projection = HMM_Perspective(field_of_view, (r32)window_w/window_h, 0.1f, 100.f);
    model = m4d(1);
    model = m4d(1);
    glEnable(GL_DEPTH_TEST);
}

void prepare_for_ui_render() {
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    projection = HMM_Perspective(field_of_view, (r32)window_w/window_h, 0.01f, 10.f);
    model = m4d(1);
    view = m4_lookat(v3(0, 0, 1), v3(0, 0, 0));

    projection_inv = m4_inverse(projection);
    view_inv = m4_inverse(view);
}

void set_shader(i16 shader) {
    if(shader >= 0) {
        active_shader = shaders[shader].id;
        glUseProgram(shaders[shader].id);
    }
    else {
        active_shader = 0;
        glUseProgram(0);
    }
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
    if(t >= 0) {
        uv_offset = v2((r32)tx/t->w, (r32)ty/t->h);
        uv_range = v2((r32)tw/t->w, (r32)th/t->h);
        glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), uv_offset.x, uv_offset.y);
        glUniform2f(glGetUniformLocation(active_shader, "uv_range"), uv_range.x, uv_range.y);
    }
}

void bind_fbo(FBO *fbo) {
    if(fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo->id);
        glViewport(0, 0, fbo->w, fbo->h);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window_w, window_h);
    }

    active_fbo = fbo;
}

void clear_fbo(FBO *fbo) {
    bind_fbo(fbo);
    {
        glClearColor(0, 0, 0, 0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    bind_fbo(NULL);
}

void bind_g_buffer(GBuffer *g) {
    if(g) {
        glBindFramebuffer(GL_FRAMEBUFFER, g->fbo);
        glViewport(0, 0, g->w, g->h);
        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, draw_buffers);
        glDisable(GL_BLEND);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glEnable(GL_BLEND);
        glViewport(0, 0, window_w, window_h);
    }
}

void clear_g_buffer(GBuffer *g) {
    bind_g_buffer(g);
    {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    bind_g_buffer(0);
}

void copy_g_buffer_depth(GBuffer *g) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, g->fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, g->w, g->h,
                      0, 0, g->w, g->h,
                      GL_DEPTH_BUFFER_BIT,
                      GL_NEAREST);
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

void draw_billboard_texture(Texture *texture, v4 tbb, v3 pos, v2 scale) {
    set_shader(SHADER_map_texture);
    {
        bind_texture(texture, tbb.x, tbb.y, tbb.z, tbb.w);

        model = m4d(1);
        model = m4_translate(model, v3(pos.x, pos.y, pos.z));

        foreach(i, 3) {
            foreach(j, 3) {
                model.Elements[i][j] = view.Elements[j][i];
            }
        }

        model = m4_scale(model, v3(scale.x, scale.y, 1));

        draw_quad();
    }
    set_shader(-1);
}

void draw_ui_rect(v4 color, v4 bb, r32 thickness) {
    set_shader(SHADER_rect);
    {
        v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;

        glUniform4f(glGetUniformLocation(active_shader, "rect_color"), color.x, color.y, color.z, color.w);
        glUniform2f(glGetUniformLocation(active_shader, "thickness"), thickness/bb.z, thickness/bb.w);

        model = m4d(1);
        model = m4_translate(model, v3(pos.x, pos.y, 0));
        model = m4_scale(model, v3(size.x, size.y, 1));
        model = m4_translate(model, v3(1, -1, 0));
        draw_quad();
    }
    set_shader(-1);
}

void draw_ui_filled_rect(v4 color, v4 bb) {
    set_shader(SHADER_rect);
    {
        v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;

        glUniform4f(glGetUniformLocation(active_shader, "rect_color"), color.x, color.y, color.z, color.w);
        glUniform2f(glGetUniformLocation(active_shader, "thickness"), -1, -1);

        model = m4d(1);
        model = m4_translate(model, v3(pos.x, pos.y, 0));
        model = m4_scale(model, v3(size.x, size.y, 1));
        model = m4_translate(model, v3(1, -1, 0));
        draw_quad();
    }
    set_shader(-1);
}

//
// @Note (Ryan)
//
// "tbb" is short for "texture bounding box", which
// is just the rectangle from the texture that you'd
// like to draw. The bounding boxes expect x/y position
// and width and height (as opposed to a secondary x/y
// positions).
//

void draw_ui_texture(Texture *texture, v4 tbb, v4 bb) {
    set_shader(SHADER_texture);
    {
        bind_texture(texture, tbb.x, tbb.y, tbb.z, tbb.w);

        v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;

        model = m4d(1);
        model = m4_translate(model, v3(pos.x, pos.y, 0));
        model = m4_scale(model, v3(size.x, size.y, 1));
        model = m4_translate(model, v3(1, -1, 0));
        draw_quad();
    }
    set_shader(-1);
}

void draw_ui_texture(Texture *texture, v4 bb) {
    draw_ui_texture(texture, v4(0, 0, texture->w, texture->h), bb);
}

void draw_ui_fbo(FBO *fbo, v4 tbb, v4 bb) {
    set_shader(SHADER_texture);
    {
        Texture texture;
        texture.id = fbo->texture;
        texture.w = fbo->w;
        texture.h = -fbo->h;
        bind_texture(&texture, tbb.x, tbb.y, tbb.z, tbb.w);

        v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;

        model = m4d(1);
        model = m4_translate(model, v3(pos.x, pos.y, 0));
        model = m4_scale(model, v3(size.x, size.y, 1));
        model = m4_translate(model, v3(1, -1, 0));
        draw_quad();
    }
    set_shader(-1);
}

void draw_ui_g_buffer(GBuffer *g, v4 bb) {
    uniform1i(uniform_loc("albedo"), 0);
    uniform1i(uniform_loc("normal"), 1);
    uniform1i(uniform_loc("depth"), 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g->textures[GBUFFER_TEXTURE_ALBEDO]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g->textures[GBUFFER_TEXTURE_NORMAL]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g->depth_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
    v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

    pos = view_inv * projection_inv * pos;
    size = view_inv * projection_inv * size;

    model = m4d(1);
    model = m4_translate(model, v3(pos.x, pos.y, 0));
    model = m4_scale(model, v3(size.x, size.y, 1));
    model = m4_translate(model, v3(1, -1, 0));

    draw_quad();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}

void draw_ui_text(const char *text, int align, v2 position) {
    set_shader(SHADER_texture);
    {
        u32 text_len = strlen(text);

        position.x += 14;

        if(align) {
            position.x -= text_len*(align == ALIGN_CENTER ? 14 : 28);
        }

        v4 pos = v4((position.x/window_w)*2 - 1, -(position.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(28.f/window_w, 32.f/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;

        model = m4d(1);
        model = m4_translate(model, v3(pos.x, pos.y, 0));

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

        bind_texture(&textures[TEX_font], 0, 0, 7, 8);

        while((c = *text++)) {
            if(c >= 65 && c <= 90) {
                tx = 7*(c-65);
                ty = 0;
            }
            else if(c >= 97 && c <= 122) {
                tx = 7*(c-97);
                ty = 8;
            }
            else if(c >= 48 && c <= 57) {
                tx = 7*(c-48);
                ty = 16;
            }
            else {
                switch(c) {
                    case '.':  { tx = 70; ty = 16; break; }
                    case ',':  { tx = 77; ty = 16; break; }
                    case '/':  { tx = 84; ty = 16; break; }
                    case '\\': { tx = 91; ty = 16; break; }
                    case '!':  { tx = 98; ty = 16; break; }
                    case '?':  { tx = 105; ty = 16; break; }
                    case ':':  { tx = 112; ty = 16; break; }
                    case ';':  { tx = 119; ty = 16; break; }
                    case '\'': { tx = 126; ty = 16; break; }
                    case '"':  { tx = 133; ty = 16; break; }
                    case '+':  { tx = 140; ty = 16; break; }
                    case '-':  { tx = 147; ty = 16; break; }
                    case '=':  { tx = 154; ty = 16; break; }
                    case '_':  { tx = 161; ty = 16; break; }

                    default: break;
                }
            }

            if(tx >= 0 && ty >= 0) {
                uv_offset = v2((r32)tx/textures[TEX_font].w, (r32)ty/textures[TEX_font].h);
                glUniform2f(glGetUniformLocation(active_shader, "uv_offset"), uv_offset.x, uv_offset.y);

                {
                    m4 char_model = HMM_Multiply(model, HMM_Scale(v3(size.x, size.y, 1)));

                    glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &char_model.Elements[0][0]);
                }

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            model = m4_translate(model, v3(size.x*2, 0, 0));

            tx = -1;
            ty = -1;
        }

        bind_texture(0);

        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);
    }
    set_shader(-1);
}
