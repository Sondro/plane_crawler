#define UI_SRC_ID 0

struct Game {
    Camera camera;
    Map map;
};

State init_game() {
    State s;
    s.type = STATE_GAME;
    s.mem = malloc(sizeof(Game));
    Game *g = (Game *)s.mem;

    g->camera.pos = v3(0, 0, 0);
    g->camera.orientation = g->camera.target_orientation = v3(0, 0, 0);
    generate_map(&g->map);

    return s;
}

void clean_up_game(State *s) {
    Game *g = (Game *)s->mem;

    clean_up_map(&g->map);

    free(s->mem);
    s->mem = 0;
    s->type = 0;
}

void update_game() {
    Game *g = (Game *)state.mem;
    
    update_camera(&g->camera);

    prepare_for_world_render(); // @World render
    {
        {
            v3 target = g->camera.pos + v3(
                cos(g->camera.orientation.x),
                sin(g->camera.orientation.y),
                sin(g->camera.orientation.x)
            );

            look_at(g->camera.pos, target);
        }
        
        look_at(v3(25, 25, 25), v3(0, 0, 0));

        draw_map(&g->map);       
    }

    prepare_for_ui_render(); // @UI Render
    {

    }
}

#undef UI_SRC_ID
