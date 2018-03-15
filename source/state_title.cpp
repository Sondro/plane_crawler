struct Title {

};

State init_title() {
    State s;
    s.type = STATE_TITLE;
    s.mem = malloc(sizeof(Title));
    Title *t = (Title *)s.mem;
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

    look_at(0, 0, 0, 32, 32, 32);

    bind_texture(tiles);
    reset_model();
    translate(32, 32, 32);
    scale(32, 32, 32);
    set_shader(&texture_quad_shader);
    draw_quad();
    set_shader(0);
}
