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
    g->camera.interpolation_rate = 0.31;
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
        ui.current_focus = 0;
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
    }
    else { // @Unpaused update
        if(key_control_down(KC_MOVE_FORWARD)) {
            g->camera.pos.x += 0.1;
        }
        if(key_control_down(KC_MOVE_BACKWARD)) {
            g->camera.pos.x -= 0.1;
        }
        if(key_control_down(KC_MOVE_LEFT)) {
            g->camera.pos.z -= 0.1;
        }
        if(key_control_down(KC_MOVE_RIGHT)) {
            g->camera.pos.z += 0.1;
        }
        if(key_control_down(KC_TURN_LEFT)) {
            g->camera.target_orientation.x -= 0.03;
        }
        if(key_control_down(KC_TURN_RIGHT)) {
            g->camera.target_orientation.x += 0.03;
        }

        g->camera.pos.y = map_coordinate_height(&g->map, g->camera.pos.x, g->camera.pos.z) + 1;
        update_camera(&g->camera);
    } 
    
    prepare_for_world_render(); // @World Render
    {
        {
            v3 target = g->camera.pos + v3(
                cos(g->camera.orientation.x),
                sin(g->camera.orientation.y),
                sin(g->camera.orientation.x)
            );

            look_at(g->camera.pos, target);
        }
        draw_map_to_texture(&g->map);
    }

    prepare_for_ui_render(); // @UI Render
    {
            
    } 

    if(g->paused) {
        draw_ui_filled_rect(v4(0, 0, 0, 0.3), v4(0, 0, window_w, window_h));
    }
}

#undef UI_SRC_ID
