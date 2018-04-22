#define UI_SRC_ID 3000

struct Dungeon {
    i8 paused, game_over;
    
    // camera data
    r32 camera_bob_sin_pos;
    Camera camera;
    
    // map/game data
    DungeonMap map;
    Player player;
    FBO render_fbo;
    
    // bg music data
    SoundSource *bg_music;
    
    // settings data
    SettingsMenu settings;
};

State init_dungeon_state() {
    State s;
    s.type = STATE_DUNGEON;
    s.mem = malloc(sizeof(Dungeon));
    Dungeon *d = (Dungeon *)s.mem;
    
    { // @Dungeon Init
        d->paused = 0;
        d->game_over = 0;
        
        d->camera_bob_sin_pos = 0;
        d->camera.pos = v3(0, 0, 0);
        d->camera.orientation = d->camera.target_orientation = v3(0, 0, 0);
        d->camera.interpolation_rate = 10;
        d->bg_music = reserve_sound_source();
        
        request_dungeon_map_assets();
        request_texture(TEX_hud);
        request_texture(TEX_hand);
        request_sound(SOUND_dungeon1);
        
        d->settings.state = -1;
        
        glfwSetInputMode(window, GLFW_CURSOR, d->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
    
    return s;
}

void clean_up_dungeon_state(State *s) {
    Dungeon *d = (Dungeon *)s->mem;
    
    { // @Dungeon Cleanup
        unrequest_texture(TEX_hud);
        unrequest_texture(TEX_hand);
        unrequest_sound(SOUND_dungeon1);
        unrequest_dungeon_map_assets();
        clean_up_dungeon_map(&d->map);
        
        stop_source(d->bg_music);
        unreserve_sound_source(d->bg_music);
    }
    
    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_dungeon_state() {
    Dungeon *d = (Dungeon *)state.mem;
    
    r32 movement_factor = 0;
    
    // @Dungeon Post Asset Loading Init
    if(first_state_frame) {
        generate_dungeon_map(&d->map);
        d->player = init_player(v2(MAP_W/2, MAP_H/2));
        play_source(d->bg_music, &sounds[SOUND_dungeon1], 1, 1, 1, AUDIO_MUSIC);
    }
    else {
        set_source_volume(d->bg_music, 1-state_t);
    }
    
    if(d->paused && !d->game_over) { // paused update
        if(d->settings.state < 0) {
            set_ui_title("PAUSED");
            
            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*3);
            {
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "RESUME")) {
                    d->paused = 0;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "SETTINGS")) {
                    d->settings.state = 0;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "RETURN TO HOUSE") && !next_state.type) {
                    next_state = init_house_state();
                }
            }
            end_block();
        }
    }
    else {
        r32 movement_speed = 30;
        
        if(d->game_over) {
            set_ui_title("GAME OVER");
            
            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*3);
            {
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, "RETURN TO HOUSE") && !next_state.type) {
                    next_state = init_title_state();
                }
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, "NEW DUNGEON") && !next_state.type) {
                    next_state = init_dungeon_state();
                }
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, "QUIT")) {
                    glfwSetWindowShouldClose(window, 1);
                }
            }
            end_block();
        }
        else {
            control_player_and_camera(&d->camera, &d->player, movement_speed);
        }
        
        { // @Dungeon Update
            if(d->player.health.val <= 0.f) {
                d->game_over = 1;
            }
            
            { // camera update
                movement_factor = (HMM_Length(d->player.box.vel) / (movement_speed*2));
                
                d->camera_bob_sin_pos += 15*delta_t;
                d->camera.pos.x = d->player.box.pos.x;
                d->camera.pos.z = d->player.box.pos.y;
                
                d->camera.pos.y = dungeon_coordinate_height(&d->map, d->camera.pos.x, d->camera.pos.z) + 0.8;
                d->camera.pos.y += sin(d->camera_bob_sin_pos)*0.42*movement_factor;
                
                update_camera(&d->camera);
                
                do_light(&d->map, d->camera.pos, v3(1, 0.9, 0.8), 15, 2);
            }
            
            update_dungeon_map(&d->map, &d->player);
        }
    }
    
    prepare_for_world_render(); // @Dungeon World Render
    {
        set_view_with_camera(&d->camera);
        draw_dungeon_map_begin(&d->map);
        
        { // Spell/hand drawing
            disable_depth();
            
            // NOTE(Ryan): We want to keep the perspective identical across
            //             all resolutions for the hand so it always fits
            //             and appears correctly
            m4 old_proj = projection;
            projection = HMM_Perspective(90.f, (r32)window_w/window_h, 0.01f, 10.f);
            
            if(d->player.attack.attacking) {
                v3 target = d->camera.pos + v3(
                    cos(d->camera.orientation.x + 0.5)*0.025,
                    sin(d->camera.orientation.y)*0.03 - 0.008 + movement_factor*sin(d->camera_bob_sin_pos)*0.002 -
                    0.015*(1-d->player.attack.transition),
                    sin(d->camera.orientation.x + 0.5)*0.025
                    );
                draw_billboard_texture(&textures[TEX_hand], v4(0, 0, 64, 64), target, v2(0.01, 0.01));
            }
            else {
                v3 target = d->camera.pos + v3(
                    cos(d->camera.orientation.x + 0.5*(1-d->player.attack.transition))*0.025,
                    sin(d->camera.orientation.y)*0.03 - 0.008 + movement_factor*sin(d->camera_bob_sin_pos)*0.002 -
                    0.015*(1-d->player.attack.transition),
                    sin(d->camera.orientation.x + 0.5*(1-d->player.attack.transition))*0.025
                    );
                draw_billboard_texture(&textures[TEX_hand], v4(64, 0, 64, 64), target, v2(0.01, 0.01));
            }
            
            projection = old_proj;
            
            enable_depth();
        }
        
        draw_dungeon_map_end(&d->map);
    }
    
    prepare_for_ui_render(); // @Dungeon UI Render
    {
        // draw health/mana bars
        
        // health bar
        draw_ui_texture(&textures[TEX_hud], v4(0, 12, 64 * d->player.health.val, 12),
                        v4(16, window_h - 120, 64*4 * d->player.health.val, 12*4));
        
        // health bar container
        draw_ui_texture(&textures[TEX_hud], v4(0, 0, 64, 12),
                        v4(16, window_h - 120, 64*4, 12*4));
        
        // mana bar
        draw_ui_texture(&textures[TEX_hud], v4(0, 24, 64 * d->player.attack.mana, 12),
                        v4(16, window_h - 64, 64*4 * d->player.attack.mana, 12*4));
        
        draw_ui_filled_rect(v4(0.8, 0.8, 0.8, 0.8), v4(16 + 64*4*d->player.attack.mana - 64*4*d->player.attack.charge, window_h - 64, 64*4 * d->player.attack.charge, 12*4));
        
        // mana bar container
        draw_ui_texture(&textures[TEX_hud], v4(0, 0, 64, 12),
                        v4(16, window_h - 64, 64*4, 12*4));
        
        // draw inventory
        foreach(i, 3) {
            if(d->player.inventory[i] >= 0) {
                draw_ui_texture(&textures[TEX_collectible], v4(collectible_data[d->player.inventory[i]].tx*8, 0, 8, 8),
                                v4(window_w - 3*64 + i*64 + 8, window_h - 64 + 8, 8*4, 8*4));
            }
            draw_ui_texture(&textures[TEX_hud], v4(64, 0, 12, 12),
                            v4(window_w - 3*64 + i*64, window_h - 64, 12*4, 12*4));
        }
    }
    
    if(d->game_over) {
        draw_ui_filled_rect(v4(0.6, 0, 0, 0.6), v4(0, 0, window_w, window_h));
    }
    
    if(d->paused) {
        draw_ui_filled_rect(v4(0, 0, 0, 0.6), v4(0, 0, window_w, window_h));
        if(d->settings.state >= 0) {
            do_settings_menu(&d->settings);
        }
    }
    
    if(key_control_pressed(KC_PAUSE) ||
       gamepad_control_pressed(GC_PAUSE)) {
        d->paused = !d->paused;
        ui.current_focus = 0;
        
        glfwSetInputMode(window, GLFW_CURSOR, d->paused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}

#undef UI_SRC_ID
