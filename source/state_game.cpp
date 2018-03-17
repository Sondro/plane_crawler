#define UI_SRC_ID 0

struct Game {
    i8 paused;
    Camera camera;
    Map map;

    FBO pause_render;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.mem = malloc(sizeof(Game));
    Game *g = (Game *)s.mem;
    
    g->paused = 0;
    g->camera.pos = v3(0, 0, 0);
    g->camera.orientation = g->camera.target_orientation = v3(0, 0, 0);
    generate_map(&g->map);
    
    g->pause_render = init_fbo(window_w, window_h);

    return s;
}

void clean_up_game(State *s) {
    Game *g = (Game *)s->mem;
    
    clean_up_fbo(&g->pause_render);

    clean_up_map(&g->map);

    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_game() {
    Game *g = (Game *)state.mem;
    
    force_fbo_size(&g->pause_render, window_w, window_h);

    if(key_control_pressed(KC_PAUSE)) {
        g->paused = !g->paused; 
    }

    if(g->paused) { // @Paused update
        begin_block(0, UI_STANDARD_W, UI_STANDARD_H*3);
        {
            if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "Resume")) {
                g->paused = 0; 
            }
            if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "Settings")) {
                
            }
            if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "Quit")) {
                next_state = init_title(); 
            }
        }
        end_block();
        
        prepare_for_ui_render(); // @UI Render (Paused)
        {
            draw_ui_fbo(&g->pause_render, v4(0, 0, window_w, window_h), v4(0, 0, window_w, window_h));
            draw_ui_filled_rect(v4(0, 0, 0, 0.5), v4(0, 0, window_w, window_h)); 
        }
    }
    else { // @Unpaused update
        update_camera(&g->camera);

        bind_fbo(&g->pause_render);
        {
            prepare_for_world_render(); // @World render
            {
                {
                    v3 target = g->camera.pos + v3(
                        cos(g->camera.orientation.x),
                        sin(g->camera.orientation.y),
                        sin(g->camera.orientation.x)
                    );

                    look_at(g->camera.pos, target);
                }
                
                look_at(v3(15, 15, 15), v3(0, 0, 0));

                draw_map(&g->map);       
            }

            prepare_for_ui_render(); // @UI Render
            {

            }
        }
        bind_fbo(0);

        draw_ui_fbo(&g->pause_render, v4(0, 0, window_w, window_h), v4(0, 0, window_w, window_h));
    }
}

#undef UI_SRC_ID
