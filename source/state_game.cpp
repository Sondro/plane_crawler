#define UI_SRC_ID 0

struct Game {
    i8 paused;
    r32 camera_bob_sin_pos;
    Camera camera;
    Player player;
    Map map;

    SettingsMenu settings;
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
    generate_map(&g->map);
    
    foreach(i, MAP_W)
    foreach(j, MAP_H) {
        if(!(tile_data[g->map.tiles[i][j]].flags & WALL) &&
           !(tile_data[g->map.tiles[i][j]].flags & PIT)) {
            g->player.pos = v2(i+0.5, j+0.5);
            g->player.vel = v2(0, 0);
        }
    }

    g->settings.state = -1;

    glfwSetInputMode(window, GLFW_CURSOR, g->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

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

    if(g->paused) { // @Paused update
        if(g->settings.state < 0) {
            set_ui_title("PAUSED");

            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*3);
            {
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "RESUME")) {
                    g->paused = 0;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "SETTINGS")) {
                    g->settings.state = 0;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "QUIT")) {
                    next_state = init_title();
                }
            }
            end_block();
        }
    }
    else { // @Unpaused update
        if(key_control_down(KC_TURN_LEFT)) {
            g->camera.target_orientation.x -= 0.05;
        }
        if(key_control_down(KC_TURN_RIGHT)) {
            g->camera.target_orientation.x += 0.05;
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

        r32 movement_length = sqrt(horizontal_movement*horizontal_movement + vertical_movement*vertical_movement);
        if(movement_length) {
            horizontal_movement /= movement_length;
            vertical_movement /= movement_length;
        }

        r32 movement_speed = 0.012;

        g->player.vel.x += cos(g->camera.orientation.x)*vertical_movement*movement_speed;
        g->player.vel.y += sin(g->camera.orientation.x)*vertical_movement*movement_speed;

        g->player.vel.x += cos(g->camera.orientation.x + PI/2)*horizontal_movement*movement_speed;
        g->player.vel.y += sin(g->camera.orientation.x + PI/2)*horizontal_movement*movement_speed;
        
        g->player.vel.x *= 0.85;
        g->player.vel.y *= 0.85;
        
        collide_entity(&g->map, &g->player.pos, &g->player.vel, 0.25);
        g->player.pos += g->player.vel;

        g->camera_bob_sin_pos += 0.25;
        g->camera.pos.x = g->player.pos.x;
        g->camera.pos.y = map_coordinate_height(&g->map, g->camera.pos.x, g->camera.pos.z) + 0.8;
        g->camera.pos.y += sin(g->camera_bob_sin_pos)*0.012*(HMM_Length(g->player.vel) / (movement_speed*2));
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

            // @TEST PROJECTILES
            if(key_pressed[KEY_SPACE]) {
                do_projectile(&g->map, PROJECTILE_FIRE, v2(g->player.pos.x, g->player.pos.y), 
                              v2(cos(g->camera.orientation.x), sin(g->camera.orientation.x)) / 5);
            }
        }

        { // @Spell drawing
            v3 target = g->camera.pos + v3(
                cos(g->camera.orientation.x+0.4)*0.3,
                sin(g->camera.orientation.y)*0.3,
                sin(g->camera.orientation.x+0.4)*0.3
            );
            /* 
            do_particle(&g->map, PARTICLE_FIRE, target + v3(0, sin(current_time*5)*0.015, 0), 
                        v3(g->player.vel.x, 0, g->player.vel.y) + 
                        v3(random32(-0.001, 0.001), random32(0.003, 0.0055), random32(-0.001, 0.001)), 
                        random32(0.05, 0.2));
            */
        }
 
        draw_map(&g->map);

    }

    prepare_for_ui_render(); // @UI Render
    {
        draw_ui_texture(&textures[TEX_HAND], v4(0, 0, 16, 16), 
                        v4(
                            window_w*(2.f/3), 
                            window_h-420 + sin(g->camera_bob_sin_pos) * 320 * HMM_Length(g->player.vel), 
                            480, 480
                        )
                       );
    }

    if(g->paused) {
        draw_ui_filled_rect(v4(0, 0, 0, 0.6), v4(0, 0, window_w, window_h));
        if(g->settings.state >= 0) {
            do_settings_menu(&g->settings);
        }
    }

    if(key_control_pressed(KC_PAUSE)) {
        g->paused = !g->paused;
        ui.current_focus = 0;

        glfwSetInputMode(window, GLFW_CURSOR, g->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}

#undef UI_SRC_ID
