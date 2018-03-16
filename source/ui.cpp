#define ui_id_equ(id1, id2) ((int)(id1*1000) == (int)(id2*1000))

#define BLOCK_MODE_VERTICAL     0
#define BLOCK_MODE_HORIZONTAL   1

#define ui_left_pressed         0
#define ui_right_pressed        0
#define ui_up_pressed           0
#define ui_down_pressed         0
#define ui_fire_pressed         0

#define ui_left_down            0
#define ui_right_down           0
#define ui_up_down              0
#define ui_down_down            0
#define ui_fire_down            0

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
    ui_id hot, active;
    
    u32 render_count;
    UIRender renders[MAX_UI_RENDER];
    
    i8 focusing;
    u32 focus_count;
    ui_id focus_ids[MAX_UI_RENDER];
    i32 current_focus;

    i8 block_mode;
    u32 current_block;

    v2 current_element_pos;

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
    ui.hot = -1;
    ui.active = -1;
    ui.render_count = 0;
    
    ui.focusing = 0;
    ui.focus_count = 0;
    ui.current_focus = 0;
    
    ui.block_mode = BLOCK_MODE_VERTICAL;
    ui.current_block = 0;
    ui.current_element_pos = v2(0, 0);

    ui.update_pos = 0;
}

void ui_begin() {
    ui.update_pos = 0;
}

void ui_end() {
    for(u32 i = ui.render_count - 1; i >= 0 && i < ui.render_count; --i) {
        if(ui.renders[i].updated) {
            if(ui_id_equ(ui.hot, ui.renders[i].id)) {
                ui.renders[i].t_hot += (1-ui.renders[i].t_hot) * 0.15;
            }
            else {
                ui.renders[i].t_active *= 0.95;
            }

            if(ui_id_equ(ui.active, ui.renders[i].id)) {
                ui.renders[i].t_active += (1-ui.renders[i].t_active) * 0.15;
            }
            else {
                ui.renders[i].t_active *= 0.95;
            }
            ui.renders[i].updated = 0;
        }
        else {
            --ui.render_count;
        }
    }
}

void begin_block(u32 block_number, r32 w, r32 h) {
    ui.focusing = (ui.current_block == block_number);
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

i8 do_button(ui_id id, r32 w, r32 h, const char *text) {
    i8 fired = 0;
    if(ui.render_count < MAX_UI_RENDER-1) {
        if(ui.focusing) {
            ui.focus_ids[ui.focus_count++] = id;
        }

        r32 x = ui.current_element_pos.x,
            y = ui.current_element_pos.y;

        if(ui.current_focus >= 0) { // @Keyboard controls
            if(ui_fire_pressed) {
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
                if(ui.hot < 0 && mouse_over) {
                    ui.hot = id;
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
