#define           MAX_UI_RENDER 256
#define MAX_UI_RENDER_TEXT_SIZE 32
#define           UI_STANDARD_W 256
#define           UI_STANDARD_H 64

#define GEN_ID (__LINE__ + UI_SRC_ID)
#define ui_id_equ(id1, id2) ((int)(id1*1000) == (int)(id2*1000))

#define BLOCK_MODE_VERTICAL     0
#define BLOCK_MODE_HORIZONTAL   1

#define ui_left_pressed         (last_key == KEY_LEFT  || last_key == KEY_A || gamepad_control_pressed(GC_MOVE_LEFT))
#define ui_right_pressed        (last_key == KEY_RIGHT || last_key == KEY_D || gamepad_control_pressed(GC_MOVE_RIGHT))
#define ui_up_pressed           (last_key == KEY_UP    || last_key == KEY_W || gamepad_control_pressed(GC_MOVE_FORWARD))
#define ui_down_pressed         (last_key == KEY_DOWN  || last_key == KEY_S || gamepad_control_pressed(GC_MOVE_BACKWARD))
#define ui_fire_pressed         (key_pressed[KEY_SPACE] || key_pressed[KEY_ENTER] || gamepad_control_pressed(GC_ATTACK))

#define ui_left_down            (key_down[KEY_LEFT]  || key_down[KEY_A] || gamepad_control_down(GC_MOVE_LEFT))
#define ui_right_down           (key_down[KEY_RIGHT] || key_down[KEY_D] || gamepad_control_down(GC_MOVE_RIGHT))
#define ui_up_down              (key_down[KEY_UP]    || key_down[KEY_W] || gamepad_control_down(GC_MOVE_FORWARD))
#define ui_down_down            (key_down[KEY_DOWN]  || key_down[KEY_S] || gamepad_control_down(GC_MOVE_BACKWARD))
#define ui_fire_down            (key_down[KEY_SPACE] || key_down[KEY_ENTER] || gamepad_control_down(GC_ATTACK))

#define play_ui_hot_sound()     (play_sound(&sounds[SOUND_ui_hot], 1, 1, 0, AUDIO_UI))
#define play_ui_fire_sound()    (play_sound(&sounds[SOUND_ui_fire], 1, 1, 0, AUDIO_UI))

typedef r64 ui_id;

enum {
    ELEMENT_BUTTON,
    ELEMENT_TOGGLER,
    ELEMENT_SLIDER,
    MAX_ELEMENT
};

struct UIRender {
    ui_id id;
    i8 type, updated;
    v4 bb;
    r32 t_hot, t_active;
    char text[MAX_UI_RENDER_TEXT_SIZE];

    union {
        struct { // @Toggler (checkbox or radio button)
            i8 checked;
        };

        struct { // @Slider
            r32 slider_val;
        };

        struct { // @Line-edit
            char empty_text[MAX_UI_RENDER_TEXT_SIZE];
        };
    };
};

global struct {
    // input data
    ui_id hot, active;
    r64 last_mouse_x, last_mouse_y;

    // renders
    u32 render_count;
    UIRender renders[MAX_UI_RENDER];

    // focus data
    i8 focusing;
    u32 focus_count;
    ui_id focus_ids[MAX_UI_RENDER];
    i32 current_focus;

    // block/auto-layout data
    i8 block_mode;
    u32 current_block;
    v2 current_block_size,
       current_element_pos;
    v4 current_block_bb;

    // ui title data
    const char *title;
    r32 title_y;

    u32 update_pos;
} ui;

UIRender init_element_render(ui_id id, i8 type, r32 x, r32 y, r32 w, r32 h, const char *text) {
    UIRender r;
    r.id = id;
    r.type = type;
    r.updated = 1;
    r.bb = v4(x, y, w, h);
    r.t_hot = 0;
    r.t_active = 0;
    snprintf(r.text, MAX_UI_RENDER_TEXT_SIZE, text);
    r.text[MAX_UI_RENDER_TEXT_SIZE-1] = 0;
    return r;
}

void init_ui() {
    // input data
    ui.hot = -1;
    ui.active = -1;
    ui.last_mouse_x = mouse_x;
    ui.last_mouse_y = mouse_y;

    // render data
    ui.render_count = 0;

    // focus data
    ui.focusing = 0;
    ui.focus_count = 0;
    ui.current_focus = 0;

    // block/auto-layout data
    ui.block_mode = BLOCK_MODE_VERTICAL;
    ui.current_block = 0;
    ui.current_element_pos = v2(0, 0);

    ui.update_pos = 0;

    request_sound(SOUND_ui_hot);
    request_sound(SOUND_ui_fire);
}

void ui_begin() {
    ui.update_pos = 0;
    ui.current_element_pos = v2(0, 0);
    ui.focus_count = 0;
    ui.title = 0;
}

void ui_end() {
    if(ui.render_count) {
        if(ui.current_focus >= 0) {
            ui.hot = ui.active = ui.focus_ids[ui.current_focus];
            if(ui_up_pressed) {
                if(!ui.current_focus--) {
                    ui.current_focus = ui.focus_count - 1;
                }
                play_ui_hot_sound();
            }
            if(ui_down_pressed) {
                if(++ui.current_focus >= (i32)ui.focus_count) {
                    ui.current_focus = 0;
                }
                play_ui_hot_sound();
            }

            if((int)ui.last_mouse_x != (int)mouse_x || (int)ui.last_mouse_y != (int)mouse_y) {
                ui.current_focus = -1;
                ui.active = -1;
                ui.hot = -1;
            }
        }
        else {
            if(ui_left_pressed || ui_right_pressed || ui_up_pressed || ui_down_pressed) {
                if(ui.hot >= 0) {
                    foreach(i, ui.focus_count) {
                        if(ui_id_equ(ui.hot, ui.focus_ids[i])) {
                            ui.current_focus = i;
                            break;
                        }
                    }
                }
                else {
                    ui.current_focus = 0;
                }
                play_ui_hot_sound();
            }
        }
    }

    ui.last_mouse_x = mouse_x;
    ui.last_mouse_y = mouse_y;

    prepare_for_ui_render();

    for(u32 i = ui.render_count - 1; i >= 0 && i < ui.render_count; --i) {
        if(ui.renders[i].updated) {
            if(ui_id_equ(ui.hot, ui.renders[i].id)) {
                ui.renders[i].t_hot += (1-ui.renders[i].t_hot) * 15*delta_t;
            }
            else {
                ui.renders[i].t_hot -= (ui.renders[i].t_hot*8*delta_t);
            }

            if(ui_id_equ(ui.active, ui.renders[i].id)) {
                ui.renders[i].t_active += (1-ui.renders[i].t_active) * 15*delta_t;
            }
            else {
                ui.renders[i].t_active *= (ui.renders[i].t_active*8*delta_t);
            }

            { // @UI Element Rendering
                v4 bb = ui.renders[i].bb;

                draw_ui_filled_rect(ui.renders[i].t_hot * v4(0.8, 0.8, 0.8, 1),
                                    v4(bb.x + bb.z/2 - (bb.z/2)*ui.renders[i].t_hot,
                                       bb.y + bb.w/2 + 24,
                                       (bb.z - 4)*ui.renders[i].t_hot, 4));
                draw_ui_filled_rect(ui.renders[i].t_hot * v4(0.8, 0.8, 0.8, 1),
                                    v4(bb.x + bb.z/2 - (bb.z/2)*ui.renders[i].t_hot,
                                       bb.y + bb.w/2 - 28,
                                       (bb.z - 4)*ui.renders[i].t_hot, 4));

                if(ui.renders[i].type == ELEMENT_SLIDER) {
                    draw_ui_filled_rect(ui.renders[i].t_hot*v4(0.2, 0.2, 0.2, 0.2) + v4(0.5, 0.5, 0.5, 0.5),
                                        v4(bb.x, bb.y + 4, (bb.z-4)*ui.renders[i].slider_val, bb.w - 8));
                }

                if(ui.renders[i].type == ELEMENT_TOGGLER) {
                    draw_ui_text(ui.renders[i].text, 0,
                                 v2(
                                     bb.x + 10 - 2*ui.renders[i].t_hot,
                                     bb.y + bb.w/2
                                 )
                                );

                    draw_ui_text(ui.renders[i].text, 0,
                                 v2(
                                     bb.x + 10 + 2*ui.renders[i].t_hot,
                                     bb.y + bb.w/2
                                 )
                                );

                    draw_ui_rect(v4(0.6, 0.6, 0.6, 0.6) + ui.renders[i].t_hot * v4(0.2, 0.2, 0.2, 0.4),
                                 v4(bb.x + bb.z-48, bb.y+bb.w/2 - 16, 32, 32), 4);

                    if(ui.renders[i].checked) {
                        draw_ui_filled_rect(v4(0.6, 0.6, 0.6, 0.6) + ui.renders[i].t_hot * v4(0.2, 0.2, 0.2, 0.4),
                                            v4(bb.x + bb.z-40, bb.y+bb.w/2 - 8, 16, 16));

                    }
                }
                else {
                    draw_ui_text(ui.renders[i].text, ALIGN_CENTER,
                                 v2(
                                     bb.x + bb.z/2 - 2*ui.renders[i].t_hot,
                                     bb.y + bb.w/2
                                 )
                                );

                    draw_ui_text(ui.renders[i].text, ALIGN_CENTER,
                                 v2(
                                     bb.x + bb.z/2 + 2*ui.renders[i].t_hot,
                                     bb.y + bb.w/2
                                 )
                                );
                }
            }
            ui.renders[i].updated = 0;
        }
        else {
            if(ui_id_equ(ui.hot, ui.renders[i].id)) {
                ui.hot = -1;
                ui.active = -1;
            }
            if(ui.current_focus >= 0) {
                ui.current_focus = 0;
            }
            memmove(ui.renders + i, ui.renders + i + 1, sizeof(UIRender) * (ui.render_count - i - 1));
            --ui.render_count;
        }
    }

    if(ui.title) {
        draw_ui_filled_rect(v4(0.5, 0.5, 0.5, 0.5), v4(0, ui.title_y - 32, window_w, 64));
        draw_ui_text(ui.title, ALIGN_CENTER, v2(window_w/2 + 2, ui.title_y));
        draw_ui_text(ui.title, ALIGN_CENTER, v2(window_w/2 - 2, ui.title_y));
    }
}

void set_ui_title(const char *title) {
    ui.title = title;
    ui.title_y = window_h/2;
}

void begin_block(u32 block_number, r32 x, r32 y, r32 w, r32 h) {
    if(ui.title && y < ui.title_y) {
        ui.title_y = y - 32;
        y += 64;
    }

    ui.current_block_bb = v4(x, y, w, h);
    ui.focusing = (ui.current_block == block_number) || !block_number;
    ui.current_element_pos = v2(x, y);
}

void begin_block(u32 block_number, r32 w, r32 h) {
    begin_block(block_number, window_w/2 - w/2, window_h/2 - h/2, w, h);
}

void end_block() {
    ui.focusing = 0;
}

void move_to_next_ui_pos(r32 w, r32 h) {
    if(ui.block_mode == BLOCK_MODE_VERTICAL) {
        ui.current_element_pos.y += h;
    }
    else {
        ui.current_element_pos.x += w;
    }
}

UIRender *find_previous_ui_render(ui_id id) {
    foreach(i, ui.render_count) {
        if(ui_id_equ(ui.renders[i].id, id)) {
            if(ui.update_pos == i) {
                return ui.renders + i;
            }
            else {
                break;
            }
        }
    }
    return 0;
}

void do_divider() {
    ui.block_mode == BLOCK_MODE_VERTICAL ?
        move_to_next_ui_pos(0, 24) :
        move_to_next_ui_pos(24, 0);
}

i8 do_button(ui_id id, r32 w, r32 h, const char *text) {
    i8 fired = 0;
    if(ui.render_count < MAX_UI_RENDER-1) {
        if(ui.focusing) {
            ui.focus_ids[ui.focus_count++] = id;
        }

        r32 x = ui.current_element_pos.x,
            y = ui.current_element_pos.y;

        if(ui.block_mode == BLOCK_MODE_VERTICAL) {
            x += (ui.current_block_bb.z-w) / 2;
        }
        else {
            y += (ui.current_block_bb.w-h) / 2;
        }

        if(ui.current_focus >= 0) { // @Keyboard controls
            if(ui_id_equ(id, ui.hot) && ui_fire_pressed) {
                fired = 1;
                play_ui_fire_sound();
            }
        }
        else { // @Mouse controls
            i8 mouse_over = 0;
            if(mouse_x >= x && mouse_x <= x+w &&
               mouse_y >= y && mouse_y <= y+h) {
                mouse_over = 1;
            }

            if(ui_id_equ(id, ui.hot)) {
                if(mouse_over) {
                    if(ui_id_equ(id, ui.active)) {
                        if(!left_mouse_down) {
                            fired = 1;
                            play_ui_fire_sound();
                            ui.active = -1;
                        }
                    }
                    else {
                        if(left_mouse_down) {
                            ui.active = id;
                        }
                    }
                }
                else {
                    ui.hot = -1;
                }
            }
            else {
                if(ui.hot < 0) {
                    if(mouse_over) {
                        ui.hot = id;
                        play_ui_hot_sound();
                    }
                }
                if(ui_id_equ(ui.active, id) && !left_mouse_down) {
                    ui.active = -1;
                }
            }
        }

        move_to_next_ui_pos(w, h);

        UIRender *prev_loop = find_previous_ui_render(id);
        if(prev_loop) {
            prev_loop->bb.x = x;
            prev_loop->bb.y = y;
            prev_loop->bb.z = w;
            prev_loop->bb.w = h;
            prev_loop->updated = 1;
            memcpy(prev_loop->text, text, MAX_UI_RENDER_TEXT_SIZE);
        }
        else {
            UIRender render = init_element_render(id, ELEMENT_BUTTON, x, y, w, h, text);
            ui.renders[ui.render_count++] = render;
        }
    }

    ++ui.update_pos;

    return fired;
}

i8 do_toggler(ui_id id, r32 w, r32 h, const char *text, i8 value) {
    i8 fired = 0;
    if(ui.render_count < MAX_UI_RENDER-1) {
        if(ui.focusing) {
            ui.focus_ids[ui.focus_count++] = id;
        }

        r32 x = ui.current_element_pos.x,
            y = ui.current_element_pos.y;

        if(ui.block_mode == BLOCK_MODE_VERTICAL) {
            x += (ui.current_block_bb.z-w) / 2;
        }
        else {
            y += (ui.current_block_bb.w-h) / 2;
        }

        if(ui.current_focus >= 0) { // @Keyboard controls
            if(ui_id_equ(id, ui.hot) && ui_fire_pressed) {
                fired = 1;
                play_ui_fire_sound();
            }
        }
        else { // @Mouse controls
            i8 mouse_over = 0;
            if(mouse_x >= x && mouse_x <= x+w &&
               mouse_y >= y && mouse_y <= y+h) {
                mouse_over = 1;
            }

            if(ui_id_equ(id, ui.hot)) {
                if(mouse_over) {
                    if(ui_id_equ(id, ui.active)) {
                        if(!left_mouse_down) {
                            fired = 1;
                            ui.active = -1;
                            play_ui_fire_sound();
                        }
                    }
                    else {
                        if(left_mouse_down) {
                            ui.active = id;
                        }
                    }
                }
                else {
                    ui.hot = -1;
                }
            }
            else {
                if(ui.hot < 0) {
                    if(mouse_over) {
                        ui.hot = id;
                        play_ui_hot_sound();
                    }
                }
                if(ui_id_equ(ui.active, id) && !left_mouse_down) {
                    ui.active = -1;
                }
            }
        }

        move_to_next_ui_pos(w, h);

        UIRender *prev_loop = find_previous_ui_render(id);
        if(prev_loop) {
            prev_loop->bb.x = x;
            prev_loop->bb.y = y;
            prev_loop->bb.z = w;
            prev_loop->bb.w = h;
            prev_loop->updated = 1;
            prev_loop->checked = value;
        }
        else {
            UIRender render = init_element_render(id, ELEMENT_TOGGLER, x, y, w, h, text);
            render.checked = value;
            ui.renders[ui.render_count++] = render;
        }
    }

    ++ui.update_pos;

    if(fired) {
        value = !value;
    }

    return value;
}

r32 do_slider(ui_id id, r32 w, r32 h, const char *text, r32 value) {
    if(ui.render_count < MAX_UI_RENDER-1) {
        if(ui.focusing) {
            ui.focus_ids[ui.focus_count++] = id;
        }

        r32 x = ui.current_element_pos.x,
            y = ui.current_element_pos.y;

        if(ui.block_mode == BLOCK_MODE_VERTICAL) {
            x += (ui.current_block_bb.z-w) / 2;
        }
        else {
            y += (ui.current_block_bb.w-h) / 2;
        }

        if(ui.current_focus >= 0) { // @Keyboard controls
            if(ui_id_equ(id, ui.hot)) {
                if(ui_right_down) {
                    value += 1.6 * delta_t;
                }
                else if(ui_left_down) {
                    value -= 1.6 * delta_t;
                }
            }
        }
        else { // @Mouse controls
            i8 mouse_over = 0;
            if(mouse_x >= x && mouse_x <= x+w &&
               mouse_y >= y && mouse_y <= y+h) {
                mouse_over = 1;
            }

            if(ui_id_equ(id, ui.hot)) {
                if(mouse_over) {
                    if(!ui_id_equ(id, ui.active)) {
                        if(left_mouse_down) {
                            ui.active = id;
                        }
                    }
                }
                else if(!ui_id_equ(ui.active, id)) {
                    ui.hot = -1;
                }
            }
            else {
                if(ui.hot < 0) {
                    if(mouse_over) {
                        ui.hot = id;
                        play_ui_hot_sound();
                    }
                }
            }

            if(ui_id_equ(ui.active, id)) {
                if(left_mouse_down) {
                    value = (mouse_x - x) / w;
                }
                else {
                    ui.active = -1;
                }
            }
        }

        if(value > 1) {
            value = 1.f;
        }
        else if(value < 0) {
            value = 0.f;
        }

        move_to_next_ui_pos(w, h);

        UIRender *prev_loop = find_previous_ui_render(id);
        if(prev_loop) {
            prev_loop->bb.x = x;
            prev_loop->bb.y = y;
            prev_loop->bb.z = w;
            prev_loop->bb.w = h;
            prev_loop->slider_val = value;
            prev_loop->updated = 1;
            strcpy(prev_loop->text, text);
        }
        else {
            UIRender render = init_element_render(id, ELEMENT_SLIDER, x, y, w, h, text);
            render.slider_val = value;
            ui.renders[ui.render_count++] = render;
        }
    }

    ++ui.update_pos;

    return value;
}

//
// @Settings Menu
//

#define UI_SRC_ID 2000

struct SettingsMenu {
    i8 state;
    i16 selected_control;
};

void do_settings_menu(SettingsMenu *s) {
    enum {
        SETTINGS_MAIN,
        SETTINGS_CONTROLS,
        SETTINGS_GRAPHICS,
        SETTINGS_AUDIO,
        SETTINGS_VIDEO,
        MAX_SETTINGS
    };

    const char *settings_titles[MAX_SETTINGS] = {
        "SETTINGS",
        "CONTROLS",
        "GRAPHICS",
        "AUDIO",
        "VIDEO",
    };

    set_ui_title(settings_titles[s->state]);

    switch(s->state) {
        case SETTINGS_MAIN: {
            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*(MAX_SETTINGS)+24);
            {
                foreach(i, MAX_SETTINGS-1) {
                    if(do_button(GEN_ID+(i/100.f), UI_STANDARD_W, UI_STANDARD_H, settings_titles[i+1])) {
                        s->state = i+1;
                        s->selected_control = -1;
                    }
                }
                do_divider();
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "BACK")) {
                    s->state = -1;
                }
            }
            end_block();
            break;
        }
        case SETTINGS_CONTROLS: {
            if(last_key && s->selected_control >= 0) {
                keyboard_used = 1;
                key_control_maps[s->selected_control] = last_key;
                last_key = 0;
                s->selected_control = -1;
            }

            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*MAX_KC);
            {
                char control_name[32] = { 0 };
                foreach(i, MAX_KC) {
                    memset(control_name, 0, 32);
                    sprintf(control_name, "%s: %s", key_control_names[i], s->selected_control == (i16)i ? "..." : key_name(key_control_maps[i]));
                    if(do_button(GEN_ID+(i/100.f), UI_STANDARD_W + 320, UI_STANDARD_H, control_name)) {
                        s->selected_control = i;
                    }
                }
                do_divider();
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "BACK")) {
                    s->state = SETTINGS_MAIN;
                }
            }
            end_block();
            break;
        }
        case SETTINGS_AUDIO: {
            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*MAX_AUDIO + UI_STANDARD_H + 24);
            {
                foreach(i, MAX_AUDIO) {
                    audio_type_data[i].volume = do_slider(GEN_ID + i/100.f, UI_STANDARD_W*2, UI_STANDARD_H, audio_type_data[i].name,
                              audio_type_data[i].volume);
                }
                do_divider();
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "BACK")) {
                    s->state = 0;
                }
            }
            end_block();
            break;
        }
        case SETTINGS_GRAPHICS: {
            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*2 + 24);
            {
                field_of_view = 70 + (do_slider(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, "FIELD OF VIEW", (field_of_view - 70) / 80)) * 80;
                do_divider();
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "BACK")) {
                    s->state = 0;
                }
            }
            end_block();
            break;
        }
        case SETTINGS_VIDEO: {
            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*3 + 24);
            {
                fullscreen = do_toggler(GEN_ID, UI_STANDARD_W+112, UI_STANDARD_H, "FULLSCREEN", fullscreen);
                char fps_str[16] = { 0 };
                sprintf(fps_str, "FPS: %i", (int)fps);
                fps = 30 + (do_slider(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, fps_str, (fps - 30) / 330)) * 330;
                do_divider();
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "BACK")) {
                    s->state = 0;
                }
            }
            end_block();
            break;
        }
        default: break;
    }
}

#undef UI_SRC_ID
