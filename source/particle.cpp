#define MAX_PARTICLE_COUNT 256
#define INSTANCE_DATA_LENGTH 18

enum {
    PARTICLE_FIRE,
    MAX_PARTICLE
};

global
struct {
    i32 texture, max_frames;
} particle_types[MAX_PARTICLE] = {
    { TEX_HAND, 16 },
};

struct ParticleSet {
    // @Particle data: x, y, z, x_vel, y_vel, z_vel, life
    u32 count;
    r32 particle_data[MAX_PARTICLE_COUNT*7],
        instance_render_data[MAX_PARTICLE_COUNT*INSTANCE_DATA_LENGTH];
    GLuint vao, instance_vbo;
};

struct ParticleMaster {
    ParticleSet sets[MAX_PARTICLE];
};

void init_particle_master(ParticleMaster *p) {
    i32 instance_data_length = INSTANCE_DATA_LENGTH;

    foreach(i, MAX_PARTICLE) {
        p->sets[i].count = 0;
        
        glGenVertexArrays(1, &p->sets[i].vao);
        glBindVertexArray(p->sets[i].vao);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glGenBuffers(1, &p->sets[i].instance_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, p->sets[i].instance_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(r32)*instance_data_length*MAX_PARTICLE,
                     p->sets[i].particle_data, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
                              INSTANCE_DATA_LENGTH*sizeof(GLfloat),
                              (void *)(0 * sizeof(GLfloat)));
        glVertexAttribDivisor(2, 1);

        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
                              INSTANCE_DATA_LENGTH*sizeof(GLfloat),
                              (void *)(4 * sizeof(GLfloat)));
        glVertexAttribDivisor(3, 1);

        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE,
                              INSTANCE_DATA_LENGTH*sizeof(GLfloat),
                              (void *)(8 * sizeof(GLfloat)));
        glVertexAttribDivisor(4, 1);

        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE,
                              INSTANCE_DATA_LENGTH*sizeof(GLfloat),
                              (void *)(12 * sizeof(GLfloat)));
        glVertexAttribDivisor(5, 1);

        glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE,
                              INSTANCE_DATA_LENGTH*sizeof(GLfloat),
                              (void *)(16 * sizeof(GLfloat)));
        glVertexAttribDivisor(6, 1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void clean_up_particle_master(ParticleMaster *p) {
    foreach(i, MAX_PARTICLE) {
        glDeleteBuffers(1, &p->sets[i].instance_vbo);
        glDeleteVertexArrays(1, &p->sets[i].vao);
    }
}

void update_particle_master(ParticleMaster *p) {
    foreach(i, MAX_PARTICLE) {
        ParticleSet *s = p->sets+i;
        r32 *particle_data = 0;
        foreach(j, s->count) {
            particle_data = s->particle_data+j*7;
            particle_data[0] += particle_data[3];
            particle_data[1] += particle_data[4];
            particle_data[2] += particle_data[5];
            particle_data[6] *= 0.95;
            if(particle_data[6] < 0.01) {
                memmove(s->particle_data+j*7, s->particle_data+(j+1)*7, 
                        sizeof(r32) * (s->count - j - 1));
                --s->count;
            }
        }
    }
}

void draw_particle_master(ParticleMaster *p) {
    foreach(i, MAX_PARTICLE) {
        ParticleSet *s = p->sets+i;
        u32 k = 0;
        r32 *particle_data;
        foreach(j, s->count) {
            particle_data = s->particle_data + j*7;
            m4 particle_model = HMM_Translate(v3(particle_data[0], particle_data[1], particle_data[2]));
            r32 progress = 1-particle_data[6],
                max_frames = (r32)particle_types[i].max_frames;

            foreach(x, 4) {
                foreach(y, 4) {
                    s->instance_render_data[k++] = particle_model.Elements[x][y];
                }
            }
            s->instance_render_data[k++] = progress;
            s->instance_render_data[k++] = max_frames;
        }
        glBindBuffer(GL_ARRAY_BUFFER, s->instance_vbo);
        glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLE_COUNT*INSTANCE_DATA_LENGTH*sizeof(r32), s->instance_render_data, GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    set_shader(SHADER_PARTICLE);
    {
        glDepthMask(GL_FALSE);
        
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &(projection.Elements[0][0]));
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &(view.Elements[0][0]));
        glUniform1i(glGetUniformLocation(active_shader, "tex"), 0);

        glActiveTexture(GL_TEXTURE0);
        
        foreach(i, MAX_PARTICLE) {
            ParticleSet *s = p->sets+i;

            glBindTexture(GL_TEXTURE_2D, textures[particle_types[i].texture].id);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glBindVertexArray(s->vao);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, quad_vertex_vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, quad_uv_vbo);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
            glEnableVertexAttribArray(4);
            glEnableVertexAttribArray(5);
            glEnableVertexAttribArray(6);
            
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, s->count);
           
            glDisableVertexAttribArray(2);
            glDisableVertexAttribArray(3);
            glDisableVertexAttribArray(4);
            glDisableVertexAttribArray(5);
            glDisableVertexAttribArray(6);
        }

        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);


        glDepthMask(GL_TRUE);
    }
    set_shader(-1);
}