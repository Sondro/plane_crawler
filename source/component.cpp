// @Box Component

struct BoxComponent {
    v2 pos, vel, size;
};

BoxComponent init_box_component(v2 pos, v2 size) {
    BoxComponent b;
    b.pos = pos;
    b.vel = v2(0, 0);
    b.size = size;
    return b;
}

void update_boxes(BoxComponent *b, i32 count) {
    foreach(i, count) {
        b[i].pos += b[i].vel * delta_t;
        b[i].vel -= b[i].vel * 6 * delta_t;
    }
}

// @Sprite Component

struct SpriteComponent {
    v3 pos;
    i16 texture, tx, ty, tw, th;
};

SpriteComponent init_sprite_component(i16 texture, i16 tx, i16 ty, i16 tw, i16 th) {
    SpriteComponent s;
    s.pos = v3(0, 0, 0);
    s.texture = texture;
    s.tx = tx;
    s.ty = ty;
    s.tw = tw;
    s.th = th;
    return s;
}

void draw_sprite_components(SpriteComponent *s, i32 count) {
    foreach(i, count) {
        draw_billboard_texture(textures + s->texture,
                               v4(s->tx, s->ty, s->tw, s->th),
                               s->pos,
                               v2(0.5 * (s->tw / 16.f), 0.5 * (s->th / 16.f)));

        ++s;
    }
}

// @Health Component

struct HealthComponent {
    r32 val, target;
};

HealthComponent init_health_component(r32 health) {
    HealthComponent h = { health, health };
    return h;
}

void update_health(HealthComponent *h, i32 count) {
    foreach(i, count) {
        h[i].val += (h[i].target - h[i].val) * delta_t;
    }
}

// @Attack Component

enum {
    ATTACK_fireball,
};

struct AttackComponent {
    v2 pos, target;
    i8 type, attacking;
    r32 charge, transition,
        mana, target_mana;
};

AttackComponent init_attack_component(i8 type) {
    AttackComponent a;
    a.type = type;
    a.attacking = 0;
    a.charge = 0;
    a.transition = 0;
    a.mana = 0;
    a.target_mana = 0;
    return a;
}

// @AI Component

enum {
    AI_idle,
    AI_roam,
    AI_melee,
    AI_ranged,
    MAX_AI
};

struct AIComponent {
    i8 state, moving;
    i32 target_id;
    v2 move_vel;
    r32 wait_start_time, wait_duration;
};

AIComponent init_ai_component(i8 default_state) {
    AIComponent a;
    a.state = default_state;
    a.moving = 0;
    a.target_id = -1;
    a.move_vel = v2(0, 0);
    a.wait_start_time = current_time;
    a.wait_duration = 5;
    return a;
}

void update_ai(AIComponent *a, i32 count) {
    foreach(i, count) {
        switch(a->state) {
            case AI_roam: { 
                if(current_time >= a->wait_start_time + a->wait_duration) { 
                    a->wait_start_time = current_time;
                    a->wait_duration = a->moving ? random32(3, 6) : random32(1, 5);
                    a->move_vel = a->moving ? v2(0, 0) : v2(random32(-5, 5), random32(-5, 5));
                    a->moving = !a->moving;
                }
                break;
            }
            default: break;
        }
        ++a;
    }
}

void move_boxes_with_ai(BoxComponent *b, AIComponent *a, i32 count) {
    foreach(i, count) {
        b->vel += (a->move_vel - b->vel) * 6 * delta_t;

        ++a;
        ++b;
    }
}
