enum {
    STATE_NULL,
    STATE_TITLE,
    STATE_DUNGEON,
    STATE_HOUSE,
    MAX_STATE
};

global struct State {
    i8 type;
    void *mem;
} state, next_state;

global r32 state_t = 1;
global i8 need_asset_refresh = 0,
first_state_frame = 1;

State init_title_state();
State init_dungeon_state();
State init_house_state();

#include "camera.cpp"
#include "component.cpp"
#include "player.cpp"
#include "light.cpp"
#include "particle.cpp"
#include "map.cpp"
#include "control.cpp"

#include "state_title.cpp"
#include "state_dungeon.cpp"
#include "state_house.cpp"

void update_state() {
    switch(state.type) {
        case STATE_TITLE:    { update_title_state(); break; }
        case STATE_DUNGEON:  { update_dungeon_state();  break; }
        case STATE_HOUSE:    { update_house_state(); break; }
        default: break;
    }
    
    first_state_frame = 0;
}

void clean_up_state() {
    switch(state.type) {
        case STATE_TITLE:    { clean_up_title_state(&state); break; }
        case STATE_DUNGEON:  { clean_up_dungeon_state(&state);  break; }
        case STATE_HOUSE:    { clean_up_house_state(&state); break; }
        default: break;
    }
}
