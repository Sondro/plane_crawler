#define UI_SRC_ID 0

struct House {
    i8 paused;

    // camera data
    r32 camera_bob_sin_pos;
    Camera camera;

    // map/game data
    HouseMap house;
    Player player;
    FBO render_fbo;

    // bg music data
    SoundSource *bg_music;

    // settings data
    SettingsMenu settings;
};

State init_house_state() {
    State s;
    s.type = STATE_HOUSE;
    s.mem = malloc(sizeof(House));
    House *h = (House *)s.mem;

    { // @Init
        h->paused = 0;

        h->camera_bob_sin_pos = 0;
        h->camera.pos = v3(0, 0, 0);
        h->camera.orientation = h->camera.target_orientation = v3(0, 0, 0);
        h->camera.interpolation_rate = 10;

        h->settings.state = -1;

        request_house_map_assets();
        request_sound(SOUND_house);
        request_sound(SOUND_footstep_wood);

        h->bg_music = reserve_sound_source();

        glfwSetInputMode(window, GLFW_CURSOR, h->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

    return s;
}

void clean_up_house_state(State *s) {
    House *h = (House *)s->mem;

    { // @Cleanup
        stop_source(h->bg_music);
        unrequest_sound(SOUND_footstep_wood);
        unrequest_sound(SOUND_house);

        unreserve_sound_source(h->bg_music);
        clean_up_house_map(&h->house);
        unrequest_house_map_assets();
    }

    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_house_state() {
    House *h = (House *)state.mem;

    r32 movement_factor = 0;

    // @Post Asset Loading Init
    if(first_state_frame) {
        init_house_map(&h->house);
        h->player = init_player(v2(MAP_W/2, MAP_H/2));
        play_source(h->bg_music, &sounds[SOUND_house], 0, 1, 1, AUDIO_MUSIC);
    }
    else {
        set_source_volume(h->bg_music, 1-state_t);
    }

    if(h->paused) { // @Paused update
        if(h->settings.state < 0) {
            set_ui_title("PAUSED");

            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*3);
            {
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "RESUME")) {
                    h->paused = 0;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "SETTINGS")) {
                    h->settings.state = 0;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "QUIT")) {
                    next_state = init_title_state();
                }
            }
            end_block();
        }
    }
    else { // @Unpaused update
        r32 movement_speed = 20;

        { // @Controls update
            if(key_control_down(KC_TURN_LEFT) || gamepad_control_down(GC_TURN_LEFT)) {
                h->camera.target_orientation.x -= 4.5*delta_t;
            }
            if(key_control_down(KC_TURN_RIGHT) || gamepad_control_down(GC_TURN_RIGHT)) {
                h->camera.target_orientation.x += 4.5*delta_t;
            }

            if(fabs(joystick_2_x) > 0.001) {
                h->camera.target_orientation.x += joystick_2_x*4.5*delta_t;
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

            h->player.box.vel.x += cos(h->camera.orientation.x)*vertical_movement*movement_speed*delta_t;
            h->player.box.vel.y += sin(h->camera.orientation.x)*vertical_movement*movement_speed*delta_t;

            h->player.box.vel.x += cos(h->camera.orientation.x + PI/2)*horizontal_movement*movement_speed*delta_t;
            h->player.box.vel.y += sin(h->camera.orientation.x + PI/2)*horizontal_movement*movement_speed*delta_t;

            if(key_control_down(KC_ATTACK) || gamepad_control_down(GC_ATTACK)) {
                h->player.attack.attacking = 1;
            }
            else {
                h->player.attack.attacking = 0;
            }

            h->player.attack.target = v2(cos(h->camera.orientation.x), sin(h->camera.orientation.x)) * 16;
        }

        { // @Camera update
            movement_factor = (HMM_Length(h->player.box.vel) / (movement_speed*2));

            r32 last_camera_bob_sin_pos = h->camera_bob_sin_pos;
            h->camera_bob_sin_pos += 10*delta_t;
            h->camera.pos.x = h->player.box.pos.x;
            h->camera.pos.z = h->player.box.pos.y;

            h->camera.pos.y = 0.5;
            h->camera.pos.y += sin(h->camera_bob_sin_pos)*0.42*movement_factor;

            update_camera(&h->camera);
            if(HMM_Length(h->player.box.vel) > 0.1 && cos(last_camera_bob_sin_pos) < 0 && cos(h->camera_bob_sin_pos) > 0) {
                play_sound(&sounds[SOUND_footstep_wood], 1, random32(0.6, 1.4), 0, AUDIO_PLAYER);
            }
        }

        do_light(&h->house, h->camera.pos, v3(1, 0.9, 0.8), 8, 2);

        // @Map update
        update_house_map(&h->house, &h->player);
    }

    prepare_for_world_render(); // @World Render
    {
        {
            v3 target = h->camera.pos + v3(
                cos(h->camera.orientation.x),
                sin(h->camera.orientation.y),
                sin(h->camera.orientation.x)
            );
            view = m4_lookat(h->camera.pos, target);
        }

        draw_house_map_begin(&h->house);
        draw_house_map_end(&h->house);
    }

    prepare_for_ui_render();
    {
        if(h->paused) {
            draw_ui_filled_rect(v4(0, 0, 0, 0.6), v4(0, 0, window_w, window_h));
            if(h->settings.state >= 0) {
                do_settings_menu(&h->settings);
            }
        }
    }

    if(key_control_pressed(KC_PAUSE) ||
       gamepad_control_pressed(GC_PAUSE)) {
        h->paused = !h->paused;
        ui.current_focus = 0;

        glfwSetInputMode(window, GLFW_CURSOR, h->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}

#undef UI_SRC_ID

