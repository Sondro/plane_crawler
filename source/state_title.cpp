struct Title {

};

State init_title() {
    State s;
    s.mem = malloc(sizeof(Title));
    Title *t = (Title *)s.mem;
}

void clean_up_title(State *s) {
    Title *t = (Title *)s->mem;
    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_title() {
    Title *t = (Title *)state.mem;
}

