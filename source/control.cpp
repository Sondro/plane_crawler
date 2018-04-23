// @Controls update

void control_player_and_camera(Camera *c, Player *p, r32 movement_speed) {
    if(key_control_down(KC_TURN_LEFT) || gamepad_control_down(GC_TURN_LEFT)) {
        c->target_orientation.x -= 4.5*delta_t;
    }
    if(key_control_down(KC_TURN_RIGHT) || gamepad_control_down(GC_TURN_RIGHT)) {
        c->target_orientation.x += 4.5*delta_t;
    }

    if(fabs(joystick_2_x) > 0.001) {
        c->target_orientation.x += joystick_2_x*4.5*delta_t;
    }

    r32 horizontal_movement = 0, vertical_movement = 0;

    if(fabs(joystick_1_x) > 0.001 || fabs(joystick_1_y) > 0.001) {
        horizontal_movement = joystick_1_x;
        vertical_movement = joystick_1_y;
    }
    else {
        if(key_control_down(KC_MOVE_FORWARD) || gamepad_control_down(GC_MOVE_FORWARD)) {
            vertical_movement += 1;
        }
        if(key_control_down(KC_MOVE_BACKWARD) || gamepad_control_down(GC_MOVE_BACKWARD)) {
            vertical_movement -= 1;
        }
        if(key_control_down(KC_MOVE_LEFT) || gamepad_control_down(GC_MOVE_LEFT)) {
            horizontal_movement -= 1;
        }
        if(key_control_down(KC_MOVE_RIGHT) || gamepad_control_down(GC_MOVE_RIGHT)) {
            horizontal_movement += 1;
        }

        r32 movement_length = sqrt(horizontal_movement*horizontal_movement + vertical_movement*vertical_movement);
        if(movement_length) {
            horizontal_movement /= movement_length;
            vertical_movement /= movement_length;
        }
    }

    p->box.vel.x += cos(c->orientation.x)*vertical_movement*movement_speed*delta_t;
    p->box.vel.y += sin(c->orientation.x)*vertical_movement*movement_speed*delta_t;

    p->box.vel.x += cos(c->orientation.x + PI/2)*horizontal_movement*movement_speed*delta_t;
    p->box.vel.y += sin(c->orientation.x + PI/2)*horizontal_movement*movement_speed*delta_t;

    if(key_control_down(KC_ATTACK) || gamepad_control_down(GC_ATTACK)) {
        p->attack.attacking = 1;
    }
    else {
        p->attack.attacking = 0;
    }

    if(key_control_down(KC_S_FIRE) /* || gamepad_control_down(GC_S_FIRE) */) {
        p->attack.type = ATTACK_fireball;

    }
    else if(key_control_down(KC_S_LIGHTNING) ) {

        p->attack.type = ATTACK_lightning;

    } else if(key_control_down(KC_S_ICE) ) {

        p->attack.type = ATTACK_ice;

    } else if(key_control_down(KC_S_WIND) ) {

        p->attack.type = ATTACK_wind;

    }
    p->attack.target = v2(cos(c->orientation.x), sin(c->orientation.x)) * 16;

    if(key_control_down(KC_I1)){    //player inventory 1
        if(p->inventory[0] == 0){
            p->health.target = p->health.val + .25;
            p->inventory[0] = -1;
        } else if(p->inventory[0] == 2){
            p->attack.mana += .25;
            p->inventory[0] = -1;
        }
    } else if(key_control_down(KC_I2)){     //player inventory 2
        if(p->inventory[1] == 0){
            p->health.target = p->health.val + .25;
            p->inventory[1] = -1;
        } else if(p->inventory[1] == 2){
            p->attack.mana += .25;
            p->inventory[1] = -1;
        }
    } else if(key_control_down(KC_I3)){    //player inventory 3
        if(p->inventory[2] == 0){
            p->health.target = p->health.val + .25;
            p->inventory[2] = -1;
        } else if(p->inventory[2] == 2){
            p->attack.mana += .25;
            p->inventory[2] = -1;
        }
    } else {
        //play noise if inventory is key
    }

}
