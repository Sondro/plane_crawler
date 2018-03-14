struct Shader {
    GLuint id;
};

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

    sprintf(vert_filename, "%s%s.%s", RESOURCES_DIRECTORY, filename, "vert");
    sprintf(frag_filename, "%s%s.%s", RESOURCES_DIRECTORY, filename, "frag");
    sprintf(info_filename, "%s%s.%s", RESOURCES_DIRECTORY, filename, "info");

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
