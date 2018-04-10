#define WALL    0x01
#define PIT     0x02
#define TILE_SET_TILE_SIZE 32

enum {
    HOUSE_TILE_floor,
    HOUSE_TILE_wall,
    HOUSE_TILE_window,
    MAX_HOUSE_TILE
};

global
struct {
    i8 tx, ty,
       flags;
} house_tile_data[MAX_HOUSE_TILE] = {
    { 0, 0, 0 },
    { 1, 0, WALL },
    { 2, 0, WALL },
};

struct HousePortal {
    i16 x, y;
    i8 difficulty;
};

struct HouseMap {
    i8 tiles[MAP_W][MAP_H];
    i8 portal_count;
    HousePortal portals[3];

    ParticleMaster particles;
    LightState lighting;

    v3 light_vector;

    u64 vertex_component_count;
    GLuint vao,
           vertex_vbo,
           uv_vbo,
           normal_vbo;

    GBuffer render;
};

void request_house_map_assets() {
    request_shader(SHADER_heightmap);
    request_shader(SHADER_world_texture);
    request_shader(SHADER_world_render);
    request_shader(SHADER_particle);
    request_texture(TEX_tile_house);

    foreach(i, MAX_PARTICLE) {
        request_texture(particle_types[i].texture);
    }
}

void unrequest_house_map_assets() {
    unrequest_shader(SHADER_heightmap);
    unrequest_shader(SHADER_world_texture);
    unrequest_shader(SHADER_world_render);
    unrequest_shader(SHADER_particle);
    unrequest_texture(TEX_tile_house);

    foreach(i, MAX_PARTICLE) {
        unrequest_texture(particle_types[i].texture);
    }
}

void do_particle(HouseMap *h, i8 type, v3 pos, v3 vel, r32 scale) {
    do_particle(&h->particles, type, pos, vel, scale);
}

void init_house_map(HouseMap *h) {
    init_particle_master(&h->particles);
    h->light_vector = v3(1, 1, 1) / sqrt(3);

    foreach(i, MAP_W)
    foreach(j, MAP_H) {
        h->tiles[i][j] = HOUSE_TILE_wall;
    }

    const char *house_map[] = {
        "##W# M E H #W##",
        "#             #",
        "W             W",
        "#             #",
        "#             #",
        "#             #",
        "#             #",
        "W             W",
        "#             #",
        "##W#  #W#######",
    };

    h->portal_count = 0;

    foreach(j, 10)
    foreach(i, 15) {
        i16 tile = HOUSE_TILE_floor;
        switch(house_map[j][i]) {
            case '#': { tile = HOUSE_TILE_wall; break; }
            case ' ': { tile = HOUSE_TILE_floor; break; }
            case 'W': { tile = HOUSE_TILE_window; break; }
            case 'E':
            case 'M':
            case 'H': {
                h->portals[h->portal_count].x = MAP_W/2 - 8 + i;
                h->portals[h->portal_count].y = MAP_H/2 - 5 + j;
                h->portals[h->portal_count++].difficulty = house_map[j][i] == 'E' ? DUNGEON_TYPE_easy :
                                                           house_map[j][i] == 'M' ? DUNGEON_TYPE_medium :
                                                           DUNGEON_TYPE_hard;
                break;
            }
            default: break;
        }
        h->tiles[MAP_W/2 - 8 + i][MAP_H/2 - 5 + j] = tile;
    }

    r32 *vertices = 0,
        *uvs = 0,
        *normals = 0;

    u32 x = 0,
        z = 0;

    while(1) {
        v3 v00 = v3(x,   0, z),
           v01 = v3(x+1, 0, z),
           v10 = v3(x,   0, z+1),
           v11 = v3(x+1, 0, z+1);

         r32 tx = (house_tile_data[h->tiles[x][z]].tx*(r32)TILE_SET_TILE_SIZE)/(r32)textures[TEX_tile_house].w,
             ty = (house_tile_data[h->tiles[x][z]].ty*(r32)TILE_SET_TILE_SIZE)/(r32)textures[TEX_tile_house].h,
             tw = (r32)TILE_SET_TILE_SIZE/textures[TEX_tile_house].w,
             th = (r32)TILE_SET_TILE_SIZE/textures[TEX_tile_house].h;

        if(house_tile_data[h->tiles[x][z]].flags & WALL) {
            if(x && !(house_tile_data[h->tiles[x-1][z]].flags & WALL)) {
                foreach(height, 2) {
                    if(h->tiles[x][z] == HOUSE_TILE_window && height) {
                        tx = 32.f / textures[TEX_tile_house].w;
                    }

                    r32 verts[] = {
                        v00.x, v00.y+height, v00.z,
                        v10.x, v10.y+height, v10.z,
                        v00.x, v00.y+height+1, v00.z,

                        v10.x, v10.y+height+1, v10.z,
                        v00.x, v00.y+height+1, v00.z,
                        v10.x, v10.y+height, v10.z
                    };

                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,

                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };

                    r32 norms[18] = { 0 };

                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);

                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(x<MAP_W-1 && !(house_tile_data[h->tiles[x+1][z]].flags & WALL)) {
                foreach(height, 2) {
                    if(h->tiles[x][z] == HOUSE_TILE_window && height) {
                        tx = 32.f / textures[TEX_tile_house].w;
                    }

                    r32 verts[] = {
                        v01.x, v01.y+height, v01.z,
                        v11.x, v11.y+height, v11.z,
                        v01.x, v01.y+height+1, v01.z,

                        v11.x, v11.y+height+1, v11.z,
                        v01.x, v01.y+height+1, v01.z,
                        v11.x, v11.y+height, v11.z
                    };

                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,

                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };

                    r32 norms[18] = { 0 };

                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);

                    foreach(i, 18) {
                        norms[i] *= -1;
                    }

                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(z && !(house_tile_data[h->tiles[x][z-1]].flags & WALL)) {
                foreach(height, 2) {
                    if(h->tiles[x][z] == HOUSE_TILE_window && height) {
                        tx = 32.f / textures[TEX_tile_house].w;
                    }

                    r32 verts[] = {
                        v00.x, v00.y+height, v00.z,
                        v01.x, v01.y+height, v01.z,
                        v00.x, v00.y+height+1, v00.z,

                        v01.x, v01.y+height+1, v01.z,
                        v00.x, v00.y+height+1, v00.z,
                        v01.x, v01.y+height, v01.z
                    };

                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,

                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };

                    r32 norms[18] = { 0 };

                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);

                    foreach(i, 18) {
                        norms[i] *= -1;
                    }

                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
            if(z < MAP_H-1 && !(house_tile_data[h->tiles[x][z+1]].flags & WALL)) {
                foreach(height, 2) {
                    if(h->tiles[x][z] == HOUSE_TILE_window && height) {
                        tx = 32.f / textures[TEX_tile_house].w;
                    }

                    r32 verts[] = {
                        v10.x, v10.y+height, v10.z,
                        v11.x, v11.y+height, v11.z,
                        v10.x, v10.y+height+1, v10.z,

                        v11.x, v11.y+height+1, v11.z,
                        v10.x, v10.y+height+1, v10.z,
                        v11.x, v11.y+height, v11.z
                    };

                    r32 uvs_[] = {
                        tx, ty+th,
                        tx+tw, ty+th,
                        tx, ty,

                        tx+tw, ty,
                        tx, ty,
                        tx+tw, ty+th
                    };

                    r32 norms[18] = { 0 };

                    calculate_heightmap_normal(verts, norms);
                    calculate_heightmap_normal(verts+9, norms+9);

                    foreach(i, sizeof(verts)/sizeof(verts[0])) {
                        da_push(vertices, verts[i]);
                    }
                    foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                        da_push(uvs, uvs_[i]);
                    }
                    foreach(i, sizeof(norms)/sizeof(norms[0])) {
                        da_push(normals, norms[i]);
                    }
                }
            }
        }
        else {
            foreach(k, 2) {
                r32 verts[] = {
                    v00.x, v00.y + k*1.5f, v00.z,
                    v01.x, v01.y + k*1.5f, v01.z,
                    v10.x, v10.y + k*1.5f, v10.z,

                    v11.x, v11.y + k*1.5f, v11.z,
                    v01.x, v01.y + k*1.5f, v01.z,
                    v10.x, v10.y + k*1.5f, v10.z,
                };

                if(k) {
                    tx = 0.f,
                    ty = 32.f / textures[TEX_tile_house].h;
                }

                r32 uvs_[] = {
                    tx, ty,
                    tx, ty+th,
                    tx+tw, ty,

                    tx+tw, ty+th,
                    tx, ty+th,
                    tx+tw, ty
                };

                r32 norms[18] = { 0 };

                calculate_heightmap_normal(verts, norms);
                calculate_heightmap_normal(verts+9, norms+9);

                if(k) {
                    foreach(l, 18) {
                        norms[l] *= -1;
                    }
                }

                foreach(i, sizeof(verts)/sizeof(verts[0])) {
                    da_push(vertices, verts[i]);
                }
                foreach(i, sizeof(uvs_)/sizeof(uvs_[0])) {
                    da_push(uvs, uvs_[i]);
                }
                foreach(i, sizeof(norms)/sizeof(norms[0])) {
                    da_push(normals, norms[i]);
                }
            }
        }

        if(++x >= MAP_W-1) {
            x = 0;
            if(++z >= MAP_H-1) {
                break;
            }
        }
    }

    glGenVertexArrays(1, &h->vao);
    glBindVertexArray(h->vao);

    glGenBuffers(1, &h->vertex_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, h->vertex_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &h->uv_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, h->uv_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(uvs), uvs, GL_STATIC_DRAW);

    glGenBuffers(1, &h->normal_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, h->normal_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(r32) * da_size(normals), normals, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    h->vertex_component_count = da_size(vertices);
    da_free(vertices);
    da_free(uvs);
    da_free(normals);

    h->render = init_g_buffer(window_w, window_h);
    init_light_state(&h->lighting);
}

void clean_up_house_map(HouseMap *h) {
    clean_up_g_buffer(&h->render);
    clean_up_particle_master(&h->particles);

    glDeleteBuffers(1, &h->normal_vbo);
    glDeleteBuffers(1, &h->uv_vbo);
    glDeleteBuffers(1, &h->vertex_vbo);
    glDeleteVertexArrays(1, &h->vao);
}

void collide_boxes_with_tiles(HouseMap *h, BoxComponent *b, i32 count) {
    i32 current_box = 0;
    while(current_box < count) {
        v2 original_vel = b->vel,
           resolution_vel = v2(0, 0);

        for(r32 i = 1; i >= 0.2; i -= 0.2) {
            v2 pos1 = b->pos + i*original_vel*delta_t;

            forrng(x, pos1.x - b->size.x/2, pos1.x + b->size.x/2+1) {
                forrng(y, pos1.y - b->size.y/2, pos1.y + b->size.y/2+1) {
                    if(x >= 0 && x < MAP_W && y >= 0 && y < MAP_H) {
                        if(house_tile_data[h->tiles[x][y]].flags & WALL ||
                           house_tile_data[h->tiles[x][y]].flags & PIT) {
                            v2 overlap = v2(100000000, 10000000);
                            i8 overlap_x = 0, overlap_y = 0;

                            if(pos1.x > x+0.5) {
                                if(x < MAP_W-1 &&
                                   !(house_tile_data[h->tiles[x+1][y]].flags & WALL) &&
                                   !(house_tile_data[h->tiles[x+1][y]].flags & PIT)) {
                                    overlap.x = (x+1) - (pos1.x-b->size.x/2);
                                    overlap_x = 1;
                                }
                            }
                            else {
                                if(x &&
                                   !(house_tile_data[h->tiles[x-1][y]].flags & WALL) &&
                                   !(house_tile_data[h->tiles[x-1][y]].flags & PIT)) {
                                    overlap.x = x-(pos1.x + b->size.x/2);
                                    overlap_x = 1;
                                }
                            }

                            if(pos1.y > y+0.5) {
                                if(y < MAP_H-1 &&
                                   !(house_tile_data[h->tiles[x][y+1]].flags & WALL) &&
                                   !(house_tile_data[h->tiles[x][y+1]].flags & PIT)) {
                                    overlap.y = (y+1) - (pos1.y-b->size.y/2);
                                    overlap_y = 1;
                                }
                            }
                            else {
                                if(y &&
                                   !(house_tile_data[h->tiles[x][y-1]].flags & WALL) &&
                                   !(house_tile_data[h->tiles[x][y-1]].flags & PIT)) {
                                    overlap.y = y-(pos1.y + b->size.y/2);
                                    overlap_y = 1;
                                }
                            }

                            v2 res = v2(0, 0);
                            if(fabs(overlap.x) < fabs(overlap.y)) {
                                if(overlap_x) {
                                    res = v2(overlap.x, 0);
                                }
                            }
                            else {
                                if(overlap_y) {
                                    res = v2(0, overlap.y);
                                }
                            }

                            if(fabs((res*i).x) >= fabs(resolution_vel.x)) {
                                resolution_vel.x = (res*i).x;
                            }
                            if(fabs((res*i).y) >= fabs(resolution_vel.y)) {
                                resolution_vel.y = (res*i).y;
                            }
                        }
                    }
                }
            }
        }
        b->vel += resolution_vel/delta_t;

        ++b;
        ++current_box;
    }
}

void do_light(HouseMap *h, v3 pos, v3 color, r32 radius, r32 intensity) {
    if(h->lighting.light_count < MAX_LIGHT) {
        Light l = { pos, color, radius, intensity };
        h->lighting.lights[h->lighting.light_count++] = l;
    }
}

void track_sprites_to_boxes(HouseMap *d, SpriteComponent *s, BoxComponent *b, i32 count) {
    foreach(i, count) {
        s->pos = v3(b->pos.x, (s->th / 16.f) * 0.5, b->pos.y);
        ++s;
        ++b;
    }
}

void update_house_map(HouseMap *h, Player *p) {
    if(p) { // @Update player
        collide_boxes_with_tiles(h, &p->box, 1);
        update_boxes(&p->box, 1);
    }

    { // @Update portals
        foreach(i, h->portal_count) {
            if((int)p->box.pos.x == h->portals[i].x && (int)p->box.pos.y == h->portals[i].y && !next_state.type) {
                next_state = init_dungeon_state();
            }

            switch(h->portals[i].difficulty) {
                case DUNGEON_TYPE_easy: {
                    do_particle(h, PARTICLE_portal_easy, v3(h->portals[i].x + 0.5, 0.5, h->portals[i].y + 0.5), v3(random32(-0.1, 0.1), random32(-0.3, 0.3), 0), random32(0.2, 0.5));
                    do_light(h, v3(h->portals[i].x + 0.5, 0.5, h->portals[i].y + 0.5), v3(0.4, 1, 0.5), 9, 3);
                    break;
                }
                case DUNGEON_TYPE_medium: {
                    do_particle(h, PARTICLE_portal_medium, v3(h->portals[i].x + 0.5, 0.5, h->portals[i].y + 0.5), v3(random32(-0.1, 0.1), random32(-0.3, 0.3), 0), random32(0.2, 0.5));
                    do_light(h, v3(h->portals[i].x + 0.5, 0.5, h->portals[i].y + 0.5), v3(0.4, 0.5, 1), 9, 3);
                    break;
                }
                case DUNGEON_TYPE_hard: {
                    do_particle(h, PARTICLE_portal_hard, v3(h->portals[i].x + 0.5, 0.5, h->portals[i].y + 0.5), v3(random32(-0.1, 0.1), random32(-0.3, 0.3), 0), random32(0.2, 0.5));
                    do_light(h, v3(h->portals[i].x + 0.5, 0.5, h->portals[i].y + 0.5), v3(1, 0.5, 0.4), 9, 3);
                    break;
                }
                default: break;
            }
        }
    }

    { // @Update particles
        update_particle_master(&h->particles);
    }

    { // @Update lighting
        i8 light_count = 0;

        set_shader(SHADER_world_render);
        {
            {
                light_count = 0;

                char str_1[24] = "lights[x].pos",
                     str_2[24] = "lights[xx].pos";

                foreach(i, h->lighting.light_count) {
                    if(light_count < 10) {
                        str_1[7] = '0' + light_count;
                        uniform3f(uniform_loc(str_1), h->lighting.lights[i].pos);
                    }
                    else {
                        str_2[7] = '0' + light_count/10;
                        str_2[8] = '0' + (light_count - 10*(light_count/10));
                        uniform3f(uniform_loc(str_2), h->lighting.lights[i].pos);
                    }

                    ++light_count;
                }
            }

            {
                light_count = 0;

                char str_1[24] = "lights[x].color",
                     str_2[24] = "lights[xx].color";

                foreach(i, h->lighting.light_count) {
                    if(light_count < 10) {
                        str_1[7] = '0' + light_count;
                        uniform3f(uniform_loc(str_1), h->lighting.lights[i].color);
                    }
                    else {
                        str_2[7] = '0' + light_count/10;
                        str_2[8] = '0' + (light_count - 10*(light_count/10));
                        uniform3f(uniform_loc(str_2), h->lighting.lights[i].color);
                    }

                    ++light_count;
                }
            }

            {
                light_count = 0;

                char str_1[24] = "lights[x].radius",
                     str_2[24] = "lights[xx].radius";

                foreach(i, h->lighting.light_count) {
                    if(light_count < 10) {
                        str_1[7] = '0' + light_count;
                        uniform1f(uniform_loc(str_1), h->lighting.lights[i].radius);
                    }
                    else {
                        str_2[7] = '0' + light_count/10;
                        str_2[8] = '0' + (light_count - 10*(light_count/10));
                        uniform1f(uniform_loc(str_2), h->lighting.lights[i].radius);
                    }

                    ++light_count;
                }
            }

            {
                light_count = 0;

                char str_1[24] = "lights[x].intensity",
                     str_2[24] = "lights[xx].intensity";

                foreach(i, h->lighting.light_count) {
                    if(light_count < 10) {
                        str_1[7] = '0' + light_count;
                        uniform1f(uniform_loc(str_1), h->lighting.lights[i].intensity);
                    }
                    else {
                        str_2[7] = '0' + light_count/10;
                        str_2[8] = '0' + (light_count - 10*(light_count/10));
                        uniform1f(uniform_loc(str_2), h->lighting.lights[i].intensity);
                    }

                    ++light_count;
                }
            }
        }

        uniform1f(uniform_loc("brightness"), 1);
        uniform3f(uniform_loc("light_vector"), h->light_vector);
        uniform1i(uniform_loc("light_count"), light_count);
        set_shader(-1);

        h->lighting.light_count = 0;
    }
}

void draw_house_map_begin(HouseMap *h) {
    force_g_buffer_size(&h->render, window_w, window_h);

    clear_g_buffer(&h->render);
    bind_g_buffer(&h->render);

    { // @Draw terrain
        model = m4d(1);

        set_shader(SHADER_heightmap);

        glBindTexture(GL_TEXTURE_2D, textures[TEX_tile_house].id);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "model"), 1, GL_FALSE, &model.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "view"), 1, GL_FALSE, &view.Elements[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(active_shader, "projection"), 1, GL_FALSE, &projection.Elements[0][0]);

        glUniform3f(glGetUniformLocation(active_shader, "light_vector"), h->light_vector.x, h->light_vector.y, h->light_vector.z);

        glBindVertexArray(h->vao);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, h->vertex_vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, h->uv_vbo);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, h->normal_vbo);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glDrawArrays(GL_TRIANGLES, 0, h->vertex_component_count);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);

        set_shader(0);
    }
}

void draw_house_map_end(HouseMap *h) {
    bind_g_buffer(0);

    m4 last_view = view;

    set_shader(SHADER_world_render);
    uniform_m4(uniform_loc("projection3d"), projection);
    uniform_m4(uniform_loc("view3d"), view);
    prepare_for_ui_render();
    glDepthMask(GL_FALSE);
    draw_ui_g_buffer(&h->render, v4(0, 0, window_w, window_h));
    glDepthMask(GL_TRUE);
    set_shader(-1);

    copy_g_buffer_depth(&h->render);

    // @Draw particles
    prepare_for_world_render();
    view = last_view;
    draw_particle_master(&h->particles);
}
