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
    }
}

// @Health Component

struct HealthComponent {
    r32 val;
};

HealthComponent init_health_component(r32 health) {
    HealthComponent h = { health };
    return h;
}

// @Attack Component

enum {
    ATTACK_FIREBALL,
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
