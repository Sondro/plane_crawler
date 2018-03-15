enum {
    STATE_NULL,
    STATE_TITLE,
    STATE_GAME,
    MAX_STATE
};

global struct State {
    i8 type;
    void *mem;
} state, next_state;

#include "noise.cpp"
#include "camera.cpp"

State init_title();
State init_game();

#include "state_title.cpp"
#include "state_game.cpp"

void update_state() {
    switch(state.type) {
        case STATE_TITLE: { update_title(); break; }
        case STATE_GAME:  { update_game();  break; }
        default: break;
    }
}

void clean_up_state() {
    switch(state.type) {
        case STATE_TITLE: { clean_up_title(&state); break; }
        case STATE_GAME:  { clean_up_game(&state);  break; }
        default: break;
    }
}
