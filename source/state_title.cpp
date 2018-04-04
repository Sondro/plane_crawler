#define UI_SRC_ID 1000

enum {
    TITLE_MAIN,
    TITLE_SAVES,
};

struct Title {
    i8 state;
    SettingsMenu settings;

    Camera camera;
    Map map;
};

State init_title() {
    State s;
    s.type = STATE_TITLE;
    s.mem = malloc(sizeof(Title));
    Title *t = (Title *)s.mem;

    t->state = TITLE_MAIN;
    t->settings.state = -1;

    t->camera.pos = v3(0, 0, 0);
    t->camera.orientation = t->camera.target_orientation = v3(PI*(2.f/3), 0, 0);
    t->camera.interpolation_rate = 0.06;

    generate_map(&t->map);

    foreach(i, MAP_W)
    foreach(j, MAP_H) {
        if(!(tile_data[t->map.tiles[i][j]].flags & WALL)) {
            t->camera.pos = v3(i+0.5, map_coordinate_height(&t->map, i+0.5, j+0.5)+2, j+0.5);
            break;
        }
    }

    t->camera.target_orientation = v3(PI*(4.f/3), 0, 0);

    return s;
}

void clean_up_title(State *s) {
    Title *t = (Title *)s->mem;

    clean_up_map(&t->map);

    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_title() {
    Title *t = (Title *)state.mem;

    update_camera(&t->camera);
    update_map(&t->map);

    // @World Render
    prepare_for_world_render();
    {
        {
            v3 target = t->camera.pos + v3(
                cos(t->camera.orientation.x),
                sin(t->camera.orientation.y),
                sin(t->camera.orientation.x)
            );
            look_at(t->camera.pos, target);
        }
        draw_map(&t->map);
    }

    // @UI Render
    prepare_for_ui_render();
    {
        if(t->settings.state < 0 && t->state == TITLE_MAIN) {
            r32 logo_w = textures[TEX_LOGO].w,
                logo_h = textures[TEX_LOGO].h;
            draw_ui_texture(&textures[TEX_LOGO], v4(window_w/2 - logo_w*2, window_h/2 - logo_h*4 + 32, logo_w*4, logo_h*4));
        }
    }

    if(t->settings.state >= 0) {
        do_settings_menu(&t->settings);
    }
    else {
        if(t->state == TITLE_MAIN) {
            begin_block(0, window_w/2 - UI_STANDARD_W/2, window_h/2 - 96, UI_STANDARD_W, UI_STANDARD_H*3);
            {
                do_divider();
                do_divider();
                do_divider();
                do_divider();
                do_divider();

                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "PLAY")) {
                    t->state = TITLE_SAVES;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "SETTINGS")) {
                    t->settings.state = 0;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "QUIT")) {
                    glfwSetWindowShouldClose(window, 1);
                }
            }
            end_block();
        }
        else if(t->state == TITLE_SAVES) {
            set_ui_title("LOAD SAVE");

            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*4 + 24);
            {
                i8 slots_full[] = {
                    file_exists("./save/save1"),
                    file_exists("./save/save2"),
                    file_exists("./save/save3")
                };

                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, slots_full[0] ? "SLOT 1" : "SLOT 1 - Empty")) {
                    next_state = init_game();
                }
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, slots_full[1] ? "SLOT 2" : "SLOT 2 - Empty")) {

                }
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, slots_full[2] ? "SLOT 3" : "SLOT 3 - Empty")) {

                }

                do_divider();

                if(do_button(GEN_ID, 256, 64, "BACK")) {
                    t->state = TITLE_MAIN;
                }
            }
            end_block();
        }
    }
}

#undef UI_SRC_ID
