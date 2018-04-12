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
    
    p->attack.target = v2(cos(c->orientation.x), sin(c->orientation.x)) * 16;
}