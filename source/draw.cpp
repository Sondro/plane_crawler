#define ALIGN_LEFT   0
#define ALIGN_RIGHT  1
#define ALIGN_CENTER 2

#define reset_model()        { model = HMM_Mat4d(1.f); }
#define translate(x, y, z)   { model = HMM_Multiply(model, HMM_Translate(HMM_Vec3(x, y, z))); }
#define scale(x, y, z)       { model = HMM_Multiply(model, HMM_Scale(HMM_Vec3(x, y, z))); }
#define rotate(a, x, y, z)   { model = HMM_Multiply(model, HMM_Rotate(a, HMM_Vec3(x, y, z))); }
#define look_at(p, t)        { view = HMM_LookAt(p, t, HMM_Vec3(0, 1, 0)); }

struct FBO {
    GLuint id, texture;
    i32 w, h;
};

global m4 model, view, projection,
          view_inv, projection_inv;

global v2 uv_offset,
          uv_range;

enum {
    SHADER_RECT,
    SHADER_TEXTURE,
    SHADER_HEIGHTMAP,
    MAX_SHADER
};

const char *shader_names[MAX_SHADER] = {
    "rect",
    "texture",
    "heightmap",
};

enum {
    TEX_ENEMY,
    TEX_TILE,
    TEX_SMALL_FONT,
    MAX_TEX
};

const char *tex_names[MAX_TEX] = {
    "enemy",
    "tile",
    "font",
};

global Shader shaders[MAX_SHADER];
global Texture textures[MAX_TEX];

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
    
    foreach(i, MAX_SHADER) {
        shaders[i] = load_shader(shader_names[i]);
    }

    foreach(i, MAX_TEX) {
        textures[i] = load_texture(tex_names[i]);
    }
    
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

void prepare_for_world_render() {
    projection = HMM_Perspective(FIELD_OF_VIEW, (r32)window_w/window_h, 1.f, 1000.f);
    reset_model();
    view = HMM_Mat4d(1);
    glEnable(GL_DEPTH_TEST);
}

void prepare_for_ui_render() {
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    projection = HMM_Perspective(FIELD_OF_VIEW, (r32)window_w/window_h, 0.01f, 10.f);
    reset_model();
    look_at(v3(0, 0, 1), v3(0, 0, 0));

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
    set_shader(SHADER_RECT);
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
        translate(1, -1, 0);
        draw_quad();
    }
    set_shader(-1);
}

void draw_ui_filled_rect(v4 color, v4 bb) {
    set_shader(SHADER_RECT);
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
        translate(1, -1, 0);
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
    set_shader(SHADER_TEXTURE);
    {
        bind_texture(texture, tbb.x, tbb.y, tbb.z, tbb.w);

        v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;
        
        reset_model();
        translate(pos.x, pos.y, 0);
        scale(size.x, size.y, 1);
        translate(1, -1, 0);
        draw_quad();
    }
    set_shader(-1);
}

void draw_ui_fbo(FBO *fbo, v4 tbb, v4 bb) {
    set_shader(SHADER_TEXTURE);
    {
        Texture texture;
        texture.id = fbo->texture;
        texture.w = fbo->w;
        texture.h = -fbo->h;
        bind_texture(&texture, tbb.x, tbb.y + texture.h, tbb.z, tbb.w);

        v4 pos = v4((bb.x/window_w)*2 - 1, -(bb.y/window_h)*2 + 1, 0, 1);
        v4 size = v4(bb.z/window_w, bb.w/window_h, 0, 0);

        pos = view_inv * projection_inv * pos;
        size = view_inv * projection_inv * size;
        
        reset_model();
        translate(pos.x, pos.y, 0);
        scale(size.x, size.y, 1);
        translate(1, -1, 0);
        draw_quad();
    }
    set_shader(-1);
}

void draw_ui_text(const char *text, int align, v2 position) {
    set_shader(SHADER_TEXTURE);
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

        bind_texture(&textures[TEX_SMALL_FONT], 0, 0, 7, 8);

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
                uv_offset = v2((r32)tx/textures[TEX_SMALL_FONT].w, (r32)ty/textures[TEX_SMALL_FONT].h);
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
    set_shader(-1);
}
