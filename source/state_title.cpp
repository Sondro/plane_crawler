struct Title {
    Camera camera;
};

State init_title() {
    State s;
    s.type = STATE_TITLE;
    s.mem = malloc(sizeof(Title));
    Title *t = (Title *)s.mem;

    t->camera.pos = v3(0, 0, 0);
    t->camera.orientation = t->camera.target_orientation = v3(0, 0, 0);

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

    update_camera(&t->camera);
    {
        v3 target = t->camera.pos +
                    v3(
                        cos(t->camera.orientation.x),
                        sin(t->camera.orientation.y),
                        sin(t->camera.orientation.x)
                    );
        look_at(t->camera.pos, target);
    }

    bind_texture(tiles);
    reset_model();
    translate(32, 32, 32);
    scale(32, 32, 32);
    set_shader(&texture_quad_shader);
    draw_quad();
    set_shader(0);
}
