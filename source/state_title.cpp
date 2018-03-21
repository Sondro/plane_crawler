#define UI_SRC_ID 1000

enum {
    TITLE_MAIN,
    TITLE_SAVES,
};

struct Title {
    i8 state;
    SettingsMenu settings;
};

State init_title() {
    State s;
    s.type = STATE_TITLE;
    s.mem = malloc(sizeof(Title));
    Title *t = (Title *)s.mem;

    t->state = TITLE_MAIN;
    t->settings.state = -1;

    return s;
}

void clean_up_title(State *s) {
    Title *t = (Title *)s->mem;
    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_title() {
    Title *t = (Title *)state.mem;
    
    if(t->settings.state >= 0) {
        do_settings_menu(&t->settings);
    }
    else {
        if(t->state == TITLE_MAIN) {
            begin_block(0, window_w/2 - UI_STANDARD_W/2, window_h/2 - 96, UI_STANDARD_W, UI_STANDARD_H*3);
            {
                do_divider();
                do_divider();
                do_divider();
                do_divider();
                do_divider();

                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "PLAY")) {
                    t->state = TITLE_SAVES;
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "SETTINGS")) {
                    t->settings.state = 0;            
                }
                if(do_button(GEN_ID, UI_STANDARD_W, UI_STANDARD_H, "QUIT")) {
                    glfwSetWindowShouldClose(window, 1);
                } 
            }
            end_block();
        }
        else if(t->state == TITLE_SAVES) {
            set_ui_title("LOAD SAVE");

            begin_block(0, UI_STANDARD_W, UI_STANDARD_H*4 + 24);
            {
                i8 slots_full[] = {
                    file_exists("./save/save1"),
                    file_exists("./save/save2"),
                    file_exists("./save/save3")
                };
                
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, slots_full[0] ? "SLOT 1" : "SLOT 1 - Empty")) {
                    next_state = init_game();
                }
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, slots_full[1] ? "SLOT 2" : "SLOT 2 - Empty")) {
                    
                }
                if(do_button(GEN_ID, UI_STANDARD_W*2, UI_STANDARD_H, slots_full[2] ? "SLOT 3" : "SLOT 3 - Empty")) {
                    
                } 

                do_divider();

                if(do_button(GEN_ID, 256, 64, "BACK")) {
                    t->state = TITLE_MAIN;
                }
            }
            end_block();
        }
    }

    // @UI Render
    prepare_for_ui_render();
    {
        if(t->settings.state < 0 && t->state == TITLE_MAIN) {
            r32 logo_w = textures[TEX_LOGO].w,
                logo_h = textures[TEX_LOGO].h;
            draw_ui_texture(&textures[TEX_LOGO], v4(window_w/2 - logo_w*2, window_h/2 - logo_h*4 + 32, logo_w*4, logo_h*4));
        }
    }
}

#undef UI_SRC_ID
