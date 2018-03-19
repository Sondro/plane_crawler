#define GEN_ID (__LINE__ + UI_SRC_ID)
#define ui_id_equ(id1, id2) ((int)(id1*1000) == (int)(id2*1000))

#define BLOCK_MODE_VERTICAL     0
#define BLOCK_MODE_HORIZONTAL   1

#define ui_left_pressed         (last_key == KEY_LEFT  || last_key == KEY_A)
#define ui_right_pressed        (last_key == KEY_RIGHT || last_key == KEY_D)
#define ui_up_pressed           (last_key == KEY_UP    || last_key == KEY_W)
#define ui_down_pressed         (last_key == KEY_DOWN  || last_key == KEY_S)
#define ui_fire_pressed         (key_pressed[KEY_SPACE] || key_pressed[KEY_ENTER])

#define ui_left_down            (key_down[KEY_LEFT]  || key_down[KEY_A])
#define ui_right_down           (key_down[KEY_RIGHT] || key_down[KEY_D])
#define ui_up_down              (key_down[KEY_UP]    || key_down[KEY_W])
#define ui_down_down            (key_down[KEY_DOWN]  || key_down[KEY_S])
#define ui_fire_down            (key_down[KEY_SPACE] || key_down[KEY_ENTER])

typedef r64 ui_id;

struct UIRender {
    ui_id id;
    i8 updated;
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
    
    u32 update_pos;
} ui;

UIRender init_element_render(ui_id id, r32 x, r32 y, r32 w, r32 h, const char *text) {
    UIRender r;
    r.id = id;
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
}

void ui_begin() {
    ui.update_pos = 0;
    ui.current_element_pos = v2(0, 0);
    ui.focus_count = 0;
}

void ui_end() {
    if(ui.current_focus >= 0) {
        ui.hot = ui.active = ui.focus_ids[ui.current_focus];
        if(ui_up_pressed) {
            if(!ui.current_focus--) {
                ui.current_focus = ui.focus_count - 1;
            }
        }
        if(ui_down_pressed) {
            if(++ui.current_focus >= (i32)ui.focus_count) {
                ui.current_focus = 0;
            }
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
        }
    }

    ui.last_mouse_x = mouse_x;
    ui.last_mouse_y = mouse_y;

    prepare_for_ui_render();

    for(u32 i = ui.render_count - 1; i >= 0 && i < ui.render_count; --i) {
        if(ui.renders[i].updated) {
            if(ui_id_equ(ui.hot, ui.renders[i].id)) {
                ui.renders[i].t_hot += (1-ui.renders[i].t_hot) * 0.25;
            }
            else {
                ui.renders[i].t_hot *= 0.7;
            }

            if(ui_id_equ(ui.active, ui.renders[i].id)) {
                ui.renders[i].t_active += (1-ui.renders[i].t_active) * 0.25;
            }
            else {
                ui.renders[i].t_active *= 0.7;
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
}

void begin_block(u32 block_number, r32 w, r32 h) {
    ui.focusing = (ui.current_block == block_number);
    ui.current_element_pos = v2(window_w/2 - w/2, window_h/2 - h/2);
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

        if(ui.current_focus >= 0) { // @Keyboard controls
            if(ui_id_equ(id, ui.hot) && ui_fire_pressed) {
                fired = 1;
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
        }
        else {
            UIRender render = init_element_render(id, x, y, w, h, text);
            ui.renders[ui.render_count++] = render;
        }
    }

    ++ui.update_pos;

    return fired;
}
