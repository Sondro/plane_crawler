#define UI_SRC_ID 0

struct Game {
    i8 paused, game_over;

    // camera data
    r32 camera_bob_sin_pos;
    Camera camera;

    // map data
    Map map;
    FBO render_fbo;

    // settings data
    SettingsMenu settings;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.mem = malloc(sizeof(Game));
    Game *g = (Game *)s.mem;

    { // @Init
        g->paused = 0;
        g->game_over = 0;

        g->camera_bob_sin_pos = 0;
        g->camera.pos = v3(0, 0, 0);
        g->camera.orientation = g->camera.target_orientation = v3(0, 0, 0);
        g->camera.interpolation_rate = 10;

        request_map_assets();
        request_texture(TEX_status_bars);
        request_texture(TEX_hand);
        
        g->settings.state = -1;

        glfwSetInputMode(window, GLFW_CURSOR, g->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

    return s;
}

void clean_up_game(State *s) {
    Game *g = (Game *)s->mem;

    { // @Cleanup
        unrequest_texture(TEX_status_bars);
        unrequest_texture(TEX_hand);

        unrequest_map_assets();
        clean_up_map(&g->map);
    }

    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_game() {
    Game *g = (Game *)state.mem;
    
    r32 movement_factor = 0;

    // @Post Asset Loading Init
    if(first_state_frame) {
        generate_map(&g->map);
    }

    if(g->game_over) {
        set_ui_title("GAME OVER");

        begin_block(0, UI_STANDARD_W, UI_STANDARD_H*3);
        {
            if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, "RETURN TO MENU")) {
                next_state = init_title();
            }
            if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, "NEW DUNGEON")) {
                next_state = init_game();
            }
            if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, "QUIT")) {
                glfwSetWindowShouldClose(window, 1);
            }
        }
        end_block();
    }
    else {
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
            r32 movement_speed = 30;

            { // @Controls update
                if(key_control_down(KC_TURN_LEFT) || gamepad_control_down(GC_TURN_LEFT)) {
                    g->camera.target_orientation.x -= 4.5*delta_t;
                }
                if(key_control_down(KC_TURN_RIGHT) || gamepad_control_down(GC_TURN_RIGHT)) {
                    g->camera.target_orientation.x += 4.5*delta_t;
                }

                if(fabs(joystick_2_x) > 0.001) {
                    g->camera.target_orientation.x += joystick_2_x*4.5*delta_t;
                }

                r32 horizontal_movement = 0,
                    vertical_movement = 0;

                if(fabs(joystick_1_x) > 0.001 || fabs(joystick_1_y) > 0.001) {
                    horizontal_movement = joystick_1_x;
                    vertical_movement = joystick_1_y;
                }
                else {
                    if(key_control_down(KC_MOVE_FORWARD) || gamepad_control_down(GC_MOVE_FORWARD)) {
                        vertical_movement += 1;
                    }
                    if(key_control_down(KC_MOVE_BACKWARD) || gamepad_control_down(GC_MOVE_BACKWARD)) {
                        vertical_movement -= 1;
                    }
                    if(key_control_down(KC_MOVE_LEFT) || gamepad_control_down(GC_MOVE_LEFT)) {
                        horizontal_movement -= 1;
                    }
                    if(key_control_down(KC_MOVE_RIGHT) || gamepad_control_down(GC_MOVE_RIGHT)) {
                        horizontal_movement += 1;
                    }

                    r32 movement_length = sqrt(horizontal_movement*horizontal_movement + vertical_movement*vertical_movement);
                    if(movement_length) {
                        horizontal_movement /= movement_length;
                        vertical_movement /= movement_length;
                    }
                }

                g->map.player.box.vel.x += cos(g->camera.orientation.x)*vertical_movement*movement_speed*delta_t;
                g->map.player.box.vel.y += sin(g->camera.orientation.x)*vertical_movement*movement_speed*delta_t;

                g->map.player.box.vel.x += cos(g->camera.orientation.x + PI/2)*horizontal_movement*movement_speed*delta_t;
                g->map.player.box.vel.y += sin(g->camera.orientation.x + PI/2)*horizontal_movement*movement_speed*delta_t;

                if(key_control_down(KC_ATTACK) || gamepad_control_down(GC_ATTACK)) {
                    g->map.player.attack.attacking = 1;
                }
                else {
                    g->map.player.attack.attacking = 0;
                }

                g->map.player.attack.target = v2(cos(g->camera.orientation.x), sin(g->camera.orientation.x)) * 16;
            } 

            { // @Camera update
                movement_factor = (HMM_Length(g->map.player.box.vel) / (movement_speed*2));
                
                g->camera_bob_sin_pos += 15*delta_t;
                g->camera.pos.x = g->map.player.box.pos.x;
                g->camera.pos.z = g->map.player.box.pos.y;

                g->camera.pos.y = map_coordinate_height(&g->map, g->camera.pos.x, g->camera.pos.z) + 0.8;
                g->camera.pos.y += sin(g->camera_bob_sin_pos)*0.42*movement_factor;

                update_camera(&g->camera);
               
                do_light(&g->map, g->camera.pos, v3(1, 0.9, 0.8), 15, 2);
            }

            // @Map update
            update_map(&g->map);
        }
    }

    prepare_for_world_render(); // @World Render
    {
        {
            v3 target = g->camera.pos + v3(
                cos(g->camera.orientation.x),
                sin(g->camera.orientation.y),
                sin(g->camera.orientation.x)
            );
            view = m4_lookat(g->camera.pos, target);
        } 

        draw_map_begin(&g->map);
        
        { // @Spell drawing 
            m4 old_proj = projection;
            projection = HMM_Perspective(90.f, (r32)window_w/window_h, 0.01f, 10.f);
            
            if(g->map.player.attack.attacking) {
                v3 target = g->camera.pos + v3(
                            cos(g->camera.orientation.x + 0.5)*0.025,
                            sin(g->camera.orientation.y)*0.03 - 0.008 + movement_factor*sin(g->camera_bob_sin_pos)*0.002 -
                            0.015*(1-g->map.player.attack.transition),
                            sin(g->camera.orientation.x + 0.5)*0.025
                );
                draw_billboard_texture(&textures[TEX_hand], v4(0, 0, 64, 64), target, v2(0.01, 0.01));
            }
            else {
                v3 target = g->camera.pos + v3(
                            cos(g->camera.orientation.x + 0.5*(1-g->map.player.attack.transition))*0.025,
                            sin(g->camera.orientation.y)*0.03 - 0.008 + movement_factor*sin(g->camera_bob_sin_pos)*0.002 -
                            0.015*(1-g->map.player.attack.transition),
                            sin(g->camera.orientation.x + 0.5*(1-g->map.player.attack.transition))*0.025
                );
                draw_billboard_texture(&textures[TEX_hand], v4(64, 0, 64, 64), target, v2(0.01, 0.01));
            }

            projection = old_proj;
        }

        draw_map_end(&g->map);
    }

    prepare_for_ui_render(); // @UI Render
    {
        // draw health/mana bars
        
        draw_ui_texture(&textures[TEX_status_bars], v4(0, 12, 64 * g->map.player.health.val, 12),
                        v4(16, window_h - 120, 64*4 * g->map.player.health.val, 12*4));

        draw_ui_texture(&textures[TEX_status_bars], v4(0, 0, 64, 12),
                        v4(16, window_h - 120, 64*4, 12*4));

        draw_ui_texture(&textures[TEX_status_bars], v4(0, 24, 64 * g->map.player.attack.mana, 12),
                        v4(16, window_h - 64, 64*4 * g->map.player.attack.mana, 12*4));

        draw_ui_texture(&textures[TEX_status_bars], v4(0, 0, 64, 12),
                        v4(16, window_h - 64, 64*4, 12*4));
    }

    if(g->game_over) {
        draw_ui_filled_rect(v4(0.6, 0, 0, 0.6), v4(0, 0, window_w, window_h));
    }

    if(g->paused) {
        draw_ui_filled_rect(v4(0, 0, 0, 0.6), v4(0, 0, window_w, window_h));
        if(g->settings.state >= 0) {
            do_settings_menu(&g->settings);
        }
    }

    if(key_control_pressed(KC_PAUSE) ||
       gamepad_control_pressed(GC_PAUSE)) {
        g->paused = !g->paused;
        ui.current_focus = 0;

        glfwSetInputMode(window, GLFW_CURSOR, g->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}

#undef UI_SRC_ID
