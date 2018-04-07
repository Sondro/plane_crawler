struct Player {
    i8 inventory[3];

    BoxComponent        box;
    HealthComponent     health;
    AttackComponent     attack;
};

Player init_player(v2 pos) {
    Player p;
    foreach(i, 3) {
        p.inventory[i] = -1;
    }
    p.box = init_box_component(pos, v2(0.4, 0.4));
    p.health = init_health_component(1);
    p.attack = init_attack_component(ATTACK_fireball);
    return p;
}
