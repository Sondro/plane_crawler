#define UI_SRC_ID 0

struct Game {
    Camera camera;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.mem = malloc(sizeof(Game));
    Game *g = (Game *)s.mem;

    g->camera.pos = v3(0, 0, 0);
    g->camera.orientation = g->camera.target_orientation = v3(0, 0, 0);

    return s;
}

void clean_up_game(State *s) {
    Game *g = (Game *)s->mem;
    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_game() {
    Game *g = (Game *)state.mem;
}

#undef UI_SRC_ID
