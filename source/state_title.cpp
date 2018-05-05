#define UI_SRC_ID 1000

enum {
    TITLE_MAIN,
    TITLE_SAVES,
};

struct Title {
    i8 state;
    SettingsMenu settings;
    
    Camera camera;
    DungeonMap map;
};

State init_title_state() {
    State s;
    s.type = STATE_TITLE;
    s.mem = malloc(sizeof(Title));
    Title *t = (Title *)s.mem;
    
    t->state = TITLE_MAIN;
    t->settings.state = -1;
    
    t->camera.pos = v3(0, 0, 0);
    t->camera.orientation = t->camera.target_orientation = v3(PI*(2.f/3), 0, 0);
    t->camera.interpolation_rate = 0.06;
    
    request_dungeon_map_assets();
    
    request_texture(TEX_logo);
    
    return s;
}

void clean_up_title_state(State *s) {
    Title *t = (Title *)s->mem;
    
    unrequest_texture(TEX_logo);
    
    unrequest_dungeon_map_assets();
    clean_up_dungeon_map(&t->map);
    
    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_title_state() {
    Title *t = (Title *)state.mem;
    
    if(first_state_frame) {
        generate_dungeon_map(&t->map);
        
        foreach(i, MAP_W)
            foreach(j, MAP_H) {
            if(!(dungeon_tile_data[t->map.tiles[i][j]].flags & WALL)) {
                t->camera.pos = v3(i+0.5, dungeon_coordinate_height(&t->map, i+0.5, j+0.5)+2, j+0.5);
                break;
            }
        }
        
        t->camera.target_orientation = v3(PI*(4.f/3), 0, 0);
    }
    
    do_light(&t->map, t->camera.pos, v3(1, 0.8, 0.6), 20, 2);
    
    update_camera(&t->camera);
    update_dungeon_map(&t->map, 0);
    
    // @Title World Render
    prepare_for_world_render();
    {
        {
            v3 target = t->camera.pos + v3(
                cos(t->camera.orientation.x),
                sin(t->camera.orientation.y),
                sin(t->camera.orientation.x)
                );
            view = m4_lookat(t->camera.pos, target);
        }
        draw_dungeon_map_begin(&t->map);
        draw_dungeon_map_end(&t->map);
    }
    
    // @Title UI Render
    prepare_for_ui_render();
    {
        if(t->settings.state < 0 && t->state == TITLE_MAIN) {
            r32 logo_w = textures[TEX_logo].w,
            logo_h = textures[TEX_logo].h;
            draw_ui_texture(&textures[TEX_logo], v4(window_w/2 - logo_w*2, window_h/2 - logo_h*4 + 32, logo_w*4, logo_h*4));
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
                do_divider(); // TODO(Ryan): what the heck is this garbage
                do_divider();
                do_divider();
                do_divider();
                
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "PLAY")) {
                    //t->state = TITLE_SAVES;
                    next_state = init_house_state();
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
                    file_exists("./saves/save1"),
                    file_exists("./saves/save2"),
                    file_exists("./saves/save3")
                };
                
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, slots_full[0] ? "SLOT 1" : "SLOT 1 - Empty") && !next_state.type) {
                    next_state = init_house_state();
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
