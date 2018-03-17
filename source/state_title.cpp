#define UI_SRC_ID 1000

enum {
    TITLE_MAIN,
    TITLE_SAVES,
};

struct Title {
    i8 state;
};

State init_title() {
    State s;
    s.type = STATE_TITLE;
    s.mem = malloc(sizeof(Title));
    Title *t = (Title *)s.mem;

    t->state = TITLE_MAIN;

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
    
    if(t->state == TITLE_MAIN) {
        begin_block(0, 256, 64*3);
        {
            if(do_button(GEN_ID, 256, 64, "Play")) {
                t->state = TITLE_SAVES;
            }
            if(do_button(GEN_ID, 256, 64, "Settings")) {
                
            }
            if(do_button(GEN_ID, 256, 64, "Quit")) {
                glfwSetWindowShouldClose(window, 1);
            } 
        }
        end_block();
    }
    else if(t->state == TITLE_SAVES) {
        begin_block(0, 256, 64*4 + 24);
        {
            i8 slots_full[] = {
                file_exists("./save/save1"),
                file_exists("./save/save2"),
                file_exists("./save/save3")
            };
            
            if(do_button(GEN_ID, 256, 64, slots_full[0] ? "Slot 1" : "Slot 1 (Empty)")) {
                next_state = init_game();
            }
            if(do_button(GEN_ID, 256, 64, slots_full[1] ? "Slot 2" : "Slot 2 (Empty)")) {
                
            }
            if(do_button(GEN_ID, 256, 64, slots_full[2] ? "Slot 3" : "Slot 3 (Empty)")) {
                
            } 

            do_divider();

            if(do_button(GEN_ID, 256, 64, "Back")) {
                t->state = TITLE_MAIN;
            }
        }
        end_block();
    }

    // @UI Render
    prepare_for_ui_render();
}

#undef UI_SRC_ID
