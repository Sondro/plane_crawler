#define shaders_begin  enum {
#define shaders_end    MAX_SHADER };
#define shader(s)      SHADER_ ## s
#include "assets_shader.cpp"
#undef shaders_begin
#undef shaders_end
#undef shader

#define shaders_begin  global const char *shader_names[] = {
#define shaders_end    };
#define shader(s)      #s
#include "assets_shader.cpp"
#undef shaders_begin
#undef shaders_end
#undef shader

#define textures_begin  enum {
#define textures_end    MAX_TEX };
#define texture(t)      TEX_ ## t
#include "assets_texture.cpp"
#undef textures_begin
#undef textures_end
#undef texture

#define textures_begin  global const char *tex_names[] = {
#define textures_end    };
#define texture(t)      #t
#include "assets_texture.cpp"
#undef textures_begin
#undef textures_end
#undef texture

#define sounds_begin  enum {
#define sounds_end    MAX_SOUND };
#define sound(s)      SOUND_ ## s
#include "assets_sound.cpp"
#undef sounds_begin
#undef sounds_end
#undef sound

#define sounds_begin  global const char *sound_names[] = {
#define sounds_end    };
#define sound(s)      #s
#include "assets_sound.cpp"
#undef sounds_begin
#undef sounds_end
#undef sound

#define request_shader(i)    { ++shader_requests[i]; }
#define unrequest_shader(i)  { --shader_requests[i]; }
#define request_texture(i)   { ++texture_requests[i]; }
#define unrequest_texture(i) { --texture_requests[i]; }
#define request_sound(i)     { ++sound_requests[i]; }
#define unrequest_sound(i)   { --sound_requests[i]; }

struct Shader {
    GLuint id;
};

struct Texture {
    GLuint id;
    i32 w, h;
};

struct Sound {
    ALuint id;
    u64 sample_count;
    u32 sample_rate;
};

global Shader  shaders[MAX_SHADER];
global i32     shader_requests[MAX_SHADER] = { 0 };

global Texture textures[MAX_TEX];
global i32     texture_requests[MAX_TEX] = { 0 };

global Sound   sounds[MAX_SOUND];
global i32     sound_requests[MAX_SOUND] = { 0 };

void load_file_into_data(const char *filename, void **data, i64 *len) {
    *data = 0;
    *len = 0;
    FILE *f = 0;
    if((f = fopen(filename, "rb"))) {
        fseek(f, 0, SEEK_END);
        *len = ftell(f);
        rewind(f);

        *data = calloc(*len, 1);
        fread(*data, 1, *len, f);
        fclose(f);
    }
    else {
        fprintf(log_file, "ERROR: Could not open \"%s\"\n", filename);
    }
}

Shader init_shader_from_data(void *vert, i64 vert_len,
                             void *frag, i64 frag_len,
                             void *info, i64 info_len) {

    Shader s;

    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    GLint result = GL_FALSE, code_len = 0;
    i32 info_log_length = 0;
    char *code = NULL;

    {
        code_len = vert_len;
        code = (char *)vert;

        fprintf(log_file, "compiling vertex shader\n");
        glShaderSource(vertex_shader_id, 1, &code, &code_len);
        glCompileShader(vertex_shader_id);
    }

    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(info_log_length) {
        char *vertex_shader_error = (char *)malloc(info_log_length * sizeof(char));
        glGetShaderInfoLog(vertex_shader_id, info_log_length, NULL, vertex_shader_error);
        fprintf(log_file, "%s\n", vertex_shader_error);
        free(vertex_shader_error);
    }

    {
        code_len = frag_len;
        code = (char *)frag;
        fprintf(log_file, "compiling fragment shader\n");
        glShaderSource(fragment_shader_id, 1, &code, &code_len);
        glCompileShader(fragment_shader_id);
    }

    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(info_log_length) {
        char *fragment_shader_error = (char *)malloc(info_log_length * sizeof(char));
        glGetShaderInfoLog(fragment_shader_id, info_log_length, NULL, fragment_shader_error);
        fprintf(log_file, "%s\n", fragment_shader_error);
        free(fragment_shader_error);
    }

    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    enum {
        READ_DIRECTION,
        READ_NAME,
        READ_INDEX
    };

    i8 read_mode = READ_DIRECTION,
       in = 0;
    u32 read_start = 0;
    char *name = NULL, *index_str = NULL;

    code = (char *)info;

    for(u64 i = 0; i < (u64)info_len; i++) {
        switch(read_mode) {
            case READ_DIRECTION: {
                if(!strncmp(code + i, "in", 2)) {
                    in = 1;
                    read_mode = READ_NAME;
                    read_start = i + 3;
                    i += 3;
                }
                else if(!strncmp(code + i, "out", 3)) {
                    in = 0;
                    read_mode = READ_NAME;
                    read_start = i + 4;
                    i += 4;
                }
                break;
            }
            case READ_NAME: {
                if(code[i] == ' ') {
                    name = (char *)malloc(sizeof(char) * (i - read_start + 1));
                    name = strncpy(name, code + read_start, (i - read_start));
                    name[i - read_start] = '\0';
                    read_start = i + 1;
                    read_mode = READ_INDEX;
                }
                break;
            }
            case READ_INDEX: {
                if(i == strlen(code) - 1 ||
                   code[i] == ' ' ||
                   code[i] == '\n') {

                    index_str = (char *)malloc(sizeof(char) * (i - read_start + 1));
                    index_str = strncpy(index_str, code + read_start, (i - read_start));
                    index_str[i - read_start] = '\0';

                    if(in) {
                        fprintf(log_file, "binding \"%s\" for input at index %i\n", name, atoi(index_str));
                        glBindAttribLocation(program_id, atoi(index_str), name);
                    }
                    else {
                        fprintf(log_file, "binding \"%s\" for output at index %i\n", name, atoi(index_str));
                        glBindFragDataLocation(program_id, atoi(index_str), name);
                    }

                    free(name);
                    free(index_str);
                    name = NULL;
                    index_str = NULL;

                    read_mode = READ_DIRECTION;
                }
                break;
            }
            default: break;
        }
    }

    fprintf(log_file, "linking program\n");
    glLinkProgram(program_id);

    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(info_log_length) {
        char *link_shader_error = (char *)malloc(info_log_length);
        glGetProgramInfoLog(program_id, info_log_length, NULL, link_shader_error);
        fprintf(log_file, "%s\n", link_shader_error);
        free(link_shader_error);
    }

    glValidateProgram(program_id);

    glDetachShader(program_id, vertex_shader_id);
    glDetachShader(program_id, fragment_shader_id);
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    s.id = program_id;

    fprintf(log_file, "shader successfully compiled\n\n");

    return s;
}

Shader load_shader(const char *filename) {
    char vert_filename[64] = { 0 },
         frag_filename[64] = { 0 },
         info_filename[64] = { 0 };

    sprintf(vert_filename, "%s%s%s.%s", ASSETS_DIR, SHADER_DIR, filename, "vert");
    sprintf(frag_filename, "%s%s%s.%s", ASSETS_DIR, SHADER_DIR, filename, "frag");
    sprintf(info_filename, "%s%s%s.%s", ASSETS_DIR, SHADER_DIR, filename, "info");

    Shader s;
    s.id = 0;

    void *vert = 0, *frag = 0, *info = 0;
    i64 vert_len = 0, frag_len = 0, info_len = 0;

    //vertex shader reading
    FILE *file = fopen(vert_filename, "r");
    if(file) {
        fseek(file, 0, SEEK_END);
        vert_len = ftell(file);
        rewind(file);

        vert = calloc(vert_len + 2, sizeof(char));
        if(!vert) {
            fprintf(log_file, "ERROR: allocation for vertex shader code failed\n");
        }
        fread(vert, 1, vert_len, file);

        ((char *)vert)[vert_len] = '\n';
        ((char *)vert)[vert_len + 1] = '\0';

        fclose(file);

        //fragment shader reading
        file = fopen(frag_filename, "r");
        if(file) {
            fseek(file, 0, SEEK_END);
            frag_len = ftell(file);
            rewind(file);

            frag = calloc(frag_len + 2, sizeof(char));
            if(!frag) {
                fprintf(log_file, "ERROR: allocation for fragment shader code failed\n");
            }
            fread(frag, 1, frag_len, file);

            ((char *)frag)[frag_len] = '\n';
            ((char *)frag)[frag_len + 1] = '\0';

            fclose(file);

            //shader info reading
            file = fopen(info_filename, "r");
            if(file) {
                fseek(file, 0, SEEK_END);
                info_len = ftell(file);
                rewind(file);

                info = calloc(info_len + 2, sizeof(char));
                if(!info) {
                    fprintf(log_file, "ERROR: allocation for shader info code failed\n");
                }
                fread(info, 1, info_len, file);

                ((char *)info)[info_len] = '\n';
                ((char *)info)[info_len + 1] = '\0';

                fclose(file);
            }
            else {
                fprintf(log_file, "ERROR: could not open \"%s\"\n", info_filename);
                return s;
            }
        }
        else {
            fprintf(log_file, "ERROR: could not open \"%s\"\n", frag_filename);
            return s;
        }
    }
    else {
        fprintf(log_file, "ERROR: could not open \"%s\"\n", vert_filename);
        return s;
    }

    s = init_shader_from_data(vert, vert_len,
                              frag, frag_len,
                              info, info_len);

    free(vert);
    free(frag);
    free(info);

    return s;
}

void clean_up_shader(Shader *s) {
    if(s->id) {
        glDeleteProgram(s->id);
        s->id = 0;
    }
}

Texture load_texture(const char *filename) {
    char pathed_filename[strlen(ASSETS_DIR) + strlen(filename) + 1] = { 0 };
    sprintf(pathed_filename, "%s%s.png", ASSETS_DIR, filename);

    Texture t = { 0, 0, 0 };

    i32 n = 0;
    u8 *data = stbi_load(pathed_filename, &t.w, &t.h, &n, 0);

    if(data) {
        glGenTextures(1, &t.id);
        glBindTexture(GL_TEXTURE_2D, t.id);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         t.w, t.h,
                         0, GL_RGBA, GL_UNSIGNED_BYTE,
                         data);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
    }
    else {
        fprintf(log_file, "ERROR: Texture at \"%s\" could not be loaded\n\n", pathed_filename);
    }

    return t;
}

void clean_up_texture(Texture *t) {
    glDeleteTextures(1, &t->id);
    t->id = 0;
    t->w = 0;
    t->h = 0;
}

Sound init_sound_from_wav_data(void *data, i64 len) {
    Sound s;

    ALenum format = 0;

    drwav *file = drwav_open_memory(data, len);
    if(file) {
        i16 *data_i = NULL;

        s.sample_count = file->totalSampleCount;
        s.sample_rate = file->sampleRate;

        data_i = (i16 *)malloc(sizeof(i16) * s.sample_count);
        r32 *sample_data_f = (r32 *)malloc(sizeof(r32) * s.sample_count);
        drwav_read_f32(file, s.sample_count, sample_data_f);

        for(u64 i = 0; i < s.sample_count; i++) {
            data_i[i] = (i16)(sample_data_f[i] * 32767.f);
        }

        if(file->bitsPerSample == 16) {
            if(file->channels == 1) {
                format = AL_FORMAT_MONO16;
            }
            else if(file->channels == 2) {
                format = AL_FORMAT_STEREO16;
            }
        }
        else if(file->bitsPerSample == 8) {
            if(file->channels == 1) {
                format = AL_FORMAT_MONO8;
            }
            else if(file->channels == 2) {
                format = AL_FORMAT_STEREO8;
            }
        }

        alGenBuffers(1, &s.id);
        alBufferData(s.id, format,
                     data_i,
                     s.sample_count * sizeof(i16),
                     s.sample_rate);

        free(data_i);
        free(sample_data_f);
        drwav_close(file);
    }

    return s;
}

Sound init_sound_from_ogg_data(void *data, i64 len) {
    Sound s;

    s.id = 0;
    s.sample_count = 0;
    s.sample_rate = 0;

    i32 channels = 0, sample_rate = 0;
    i16 *data_i = NULL;
    ALenum format = 0;

    s.sample_count = stb_vorbis_decode_memory((u8 *)data, len, &channels, &sample_rate, &data_i);
    if(s.sample_count > 0) {
        s.sample_rate = sample_rate;

        if(channels == 1) {
            format = AL_FORMAT_MONO16;
        }
        else if(channels == 2) {
            format = AL_FORMAT_STEREO16;
        }

        alGenBuffers(1, &s.id);
        alBufferData(s.id, format,
                     data_i,
                     s.sample_count * channels * sizeof(i16),
                     s.sample_rate);

        free(data_i);
    }

    return s;
}

Sound load_sound(const char *filename) {
    i8 ogg = 0;
    Sound s;
    s.id = 0;

    char full_filename[64] = { 0 };
    sprintf(full_filename, "%s%s%s.wav", ASSETS_DIR, SOUND_DIR, filename);
    if(!file_exists(full_filename)) {
        sprintf(full_filename, "%s%s%s.ogg", ASSETS_DIR, SOUND_DIR, filename);
        ogg = 1;
    }

    void *data = 0;
    i64 len = 0;
    load_file_into_data(full_filename, &data, &len);

    if(data) {
        s = ogg ? init_sound_from_ogg_data(data, len) :
                  init_sound_from_wav_data(data, len);
        free(data);
    }

    return s;
}

void clean_up_sound(Sound *s) {
    if(s->id) {
        alDeleteBuffers(1, &s->id);
        s->id = 0;
    }
}

void init_assets() {
    foreach(i, MAX_SHADER) {
        shaders[i].id = 0;
    }
    foreach(i, MAX_TEX) {
        textures[i].id = 0;
    }
    foreach(i, MAX_SOUND) {
        sounds[i].id = 0;
    }
}

void clean_up_assets() {
    foreach(i, MAX_SHADER) {
        clean_up_shader(shaders+i);
    }
    foreach(i, MAX_TEX) {
        clean_up_texture(textures+i);
    }
    foreach(i, MAX_SOUND) {
        clean_up_sound(sounds+i);
    }
}

void update_assets() {
    foreach(i, MAX_SHADER) {
        if(!shaders[i].id && shader_requests[i]) {
            shaders[i] = load_shader(shader_names[i]);
        }
        else if(shaders[i].id && !shader_requests[i]) {
            clean_up_shader(shaders + i);
        }
    }

    foreach(i, MAX_TEX) {
        if(!textures[i].id && texture_requests[i]) {
            textures[i] = load_texture(tex_names[i]);
        }
        else if(textures[i].id && !texture_requests[i]) {
            clean_up_texture(textures + i);
        }
    }

    foreach(i, MAX_SOUND) {
        if(!sounds[i].id && sound_requests[i]) {
            sounds[i] = load_sound(sound_names[i]);
        }
        else if(sounds[i].id && !sound_requests[i]) {
            clean_up_sound(sounds + i);
        }
    }
}
