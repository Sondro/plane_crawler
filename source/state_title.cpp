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
    t->camera.interpolation_rate = 0.1;

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

    if(left_mouse_down) {
        t->camera.target_orientation.x += 3;
    }
    if(right_mouse_down) {
        t->camera.target_orientation.x -= 3;
    }

    update_camera(&t->camera);

    // @World Render
    prepare_for_world_render();
    {
        {
            v3 target = t->camera.pos +
                        v3(
                            cos(deg2rad(t->camera.orientation.x)),
                            sin(deg2rad(t->camera.orientation.y)),
                            sin(deg2rad(t->camera.orientation.x))
                        );
            look_at(t->camera.pos, target);
        }
    }

    // @UI Render
    prepare_for_ui_render();
    //draw_ui_rect(v4(1, 0, 0, 1), v4(mouse_x, mouse_y, 64, 64), 4);
    draw_ui_text("Hello, World!", 0, v2(mouse_x, mouse_y + 64));
}
