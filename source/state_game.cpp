#define UI_SRC_ID 0

struct Game {
    i8 paused, game_over;

    // camera data
    r32 camera_bob_sin_pos;
    Camera camera;

    // map data
    Map map;

    // settings data
    SettingsMenu settings;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.mem = malloc(sizeof(Game));
    Game *g = (Game *)s.mem;

    g->paused = 0;
    g->game_over = 0;

    g->camera_bob_sin_pos = 0;
    g->camera.pos = v3(0, 0, 0);
    g->camera.orientation = g->camera.target_orientation = v3(0, 0, 0);
    g->camera.interpolation_rate = 800*delta_t;

    generate_map(&g->map);

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
                if(key_control_down(KC_TURN_LEFT)) {
                    g->camera.target_orientation.x -= 4.5*delta_t;
                }
                if(key_control_down(KC_TURN_RIGHT)) {
                    g->camera.target_orientation.x += 4.5*delta_t;
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

                g->map.player.box.vel.x += cos(g->camera.orientation.x)*vertical_movement*movement_speed*delta_t;
                g->map.player.box.vel.y += sin(g->camera.orientation.x)*vertical_movement*movement_speed*delta_t;

                g->map.player.box.vel.x += cos(g->camera.orientation.x + PI/2)*horizontal_movement*movement_speed*delta_t;
                g->map.player.box.vel.y += sin(g->camera.orientation.x + PI/2)*horizontal_movement*movement_speed*delta_t;

                if(key_down[KEY_SPACE]) {
                    g->map.player.attack.attacking = 1;
                }
                else {
                    g->map.player.attack.attacking = 0;
                }

                g->map.player.attack.target = v2(cos(g->camera.orientation.x), sin(g->camera.orientation.x)) * 24;
            }

            // @Map update
            update_map(&g->map);

            { // @Camera update
                g->camera_bob_sin_pos += 15*delta_t;
                g->camera.pos.x = g->map.player.box.pos.x;
                g->camera.pos.z = g->map.player.box.pos.y;

                g->camera.pos.y = map_coordinate_height(&g->map, g->camera.pos.x, g->camera.pos.z) + 0.8;
                g->camera.pos.y += sin(g->camera_bob_sin_pos)*0.42*(HMM_Length(g->map.player.box.vel) / (movement_speed*2));

                update_camera(&g->camera);
            }
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
            look_at(g->camera.pos, target);
        }

        { // @Spell drawing
            /*
            if(g->player.casting_spell) {
                v3 target = g->camera.pos + v3(
                    cos(g->camera.orientation.x + 0.7)*0.3,
                    sin(g->camera.orientation.y - 0.2)*0.3 - 0.1,
                    sin(g->camera.orientation.x + 0.7)*0.3
                );

                do_particle(&g->map, PARTICLE_FIRE, target + v3(0, sin(current_time*5)*0.015, 0),
                            v3(g->player.vel.x, 0, g->player.vel.y) +
                            v3(random32(-0.001, 0.001), random32(0.003, 0.0055), random32(-0.001, 0.001)),
                            random32(0.05, 0.1));
            }
            */
        }

        draw_map(&g->map);

    }

    prepare_for_ui_render(); // @UI Render
    {
        // draw health/mana bars
        /*
        draw_ui_texture(&textures[TEX_STATUS_BARS], v4(0, 12, 64 * g->player.health, 12),
                        v4(16, window_h - 120, 64*4 * g->player.health, 12*4));

        draw_ui_texture(&textures[TEX_STATUS_BARS], v4(0, 0, 64, 12),
                        v4(16, window_h - 120, 64*4, 12*4));

        draw_ui_texture(&textures[TEX_STATUS_BARS], v4(0, 24, 64 * g->player.mana, 12),
                        v4(16, window_h - 64, 64*4 * g->player.mana, 12*4));

        draw_ui_texture(&textures[TEX_STATUS_BARS], v4(0, 0, 64, 12),
                        v4(16, window_h - 64, 64*4, 12*4));
        */
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

    if(key_control_pressed(KC_PAUSE)) {
        g->paused = !g->paused;
        ui.current_focus = 0;

        glfwSetInputMode(window, GLFW_CURSOR, g->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}

#undef UI_SRC_ID
