struct Game {

};

State init_game() {
    State s;
    s.mem = malloc(sizeof(Game));
    Game *g = (Game *)s.mem;
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
