#define UI_SRC_ID 0

struct Game {
    i8 paused;
    r32 camera_bob_sin_pos;
    Camera camera;
    Player player;
    Map map;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.mem = malloc(sizeof(Game));
    Game *g = (Game *)s.mem;
    
    g->paused = 0;
    g->camera_bob_sin_pos = 0;
    g->camera.pos = v3(0, 0, 0);
    g->camera.orientation = g->camera.target_orientation = v3(0, 0, 0);
    g->camera.interpolation_rate = 0.31;
    g->player.pos = v2(64, 64);
    g->player.vel = v2(0, 0);
    generate_map(&g->map);
    
    return s;
}

void clean_up_game(State *s) {
    Game *g = (Game *)s->mem;
     
    clean_up_map(&g->map);

    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_game() {
    Game *g = (Game *)state.mem;
     
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
        if(key_control_down(KC_TURN_LEFT)) {
            g->camera.target_orientation.x -= 0.03;
        }
        if(key_control_down(KC_TURN_RIGHT)) {
            g->camera.target_orientation.x += 0.03;
        }

        r32 horizontal_movement = 0,
            vertical_movement = 0;

        if(key_control_down(KC_MOVE_FORWARD)) {
            vertical_movement += 1;
        }
        if(key_control_down(KC_MOVE_BACKWARD)) {
            vertical_movement -= 1;
        }
        if(key_control_down(KC_MOVE_LEFT)) {
            horizontal_movement -= 1;
        }
        if(key_control_down(KC_MOVE_RIGHT)) {
            horizontal_movement += 1;
        }  
        
        g->player.vel.x += cos(g->camera.orientation.x)*vertical_movement*0.02;
        g->player.vel.y += sin(g->camera.orientation.x)*vertical_movement*0.02;

        g->player.vel.x += cos(g->camera.orientation.x + PI/2)*horizontal_movement*0.02;
        g->player.vel.y += sin(g->camera.orientation.x + PI/2)*horizontal_movement*0.02;
         
        g->player.vel.x *= 0.85;
        g->player.vel.y *= 0.85;

        g->player.pos += g->player.vel;
        
        g->camera_bob_sin_pos += 0.25;
        g->camera.pos.x = g->player.pos.x;
        g->camera.pos.y = map_coordinate_height(&g->map, g->camera.pos.x, g->camera.pos.z) + 1.5;
        g->camera.pos.y += sin(g->camera_bob_sin_pos)*0.018*(HMM_Length(g->player.vel) / 0.04);
        g->camera.pos.z = g->player.pos.y;

        update_camera(&g->camera);
        update_map(&g->map);
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
        draw_map(&g->map);
    }

    prepare_for_ui_render(); // @UI Render
    {
         
    } 

    if(g->paused) {
        draw_ui_filled_rect(v4(0, 0, 0, 0.3), v4(0, 0, window_w, window_h));
    }
}

#undef UI_SRC_ID
