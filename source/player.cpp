struct Player {
    v2 pos,
       vel;

    r32 health,
        mana;

    i8 casting_spell;
    r32 spell_strength, cast_t;
};

Player init_player(v2 pos) {
    Player p;
    p.pos = pos;
    p.vel = v2(0, 0);
    p.health = 1;
    p.mana = 1;
    p.casting_spell = 0;
    p.spell_strength = 0;
    p.cast_t = 0;
    return p;
}
