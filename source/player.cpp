struct Player {
    BoxComponent        box;
    HealthComponent     health;
    AttackComponent     attack;
};

Player init_player(v2 pos) {
    Player p;
    p.box = init_box_component(pos, v2(0.4, 0.4));
    p.health = init_health_component(1);
    p.attack = init_attack_component(ATTACK_FIREBALL);
    return p;
}
