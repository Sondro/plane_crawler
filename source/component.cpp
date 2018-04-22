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
    ATTACK_lightning,
    ATTACK_ice,
    ATTACK_wind,
    ATTACK_jelly,
    ATTACK_dark,
    MAX_ATTACK
};

global struct {
    i32 projectile_type;
} attack_data[MAX_ATTACK] = {
    { PROJECTILE_fire, },
    { PROJECTILE_lightning, },
    { PROJECTILE_ice, },
    { PROJECTILE_wind, },
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
    AI_STATE_idle,
    AI_STATE_roam,
    AI_STATE_attack,
    MAX_AI_STATE
};

struct AIComponent {
    v2 pos;
    i8 attack_type, state, moving;
    i32 target_id;
    v2 move_vel;
    r32 wait_start_time, wait_duration;
};

AIComponent init_ai_component(i8 default_state, i8 attack_type) {
    AIComponent a;
    a.pos = v2(0, 0);
    a.state = default_state;
    a.attack_type = attack_type;
    a.moving = 0;
    a.target_id = 0;
    a.move_vel = v2(0, 0);
    a.wait_start_time = current_time;
    a.wait_duration = 5;
    return a;
}

void move_boxes_with_ai(BoxComponent *b, AIComponent *a, i32 count) {
    foreach(i, count) {
        a->pos = b->pos;
        b->vel += (a->move_vel - b->vel) * 6 * delta_t;

        ++a;
        ++b;
    }
}
