global r64 mouse_x, mouse_y;
global i8 mouse_state;

//
// @Note (Ryan) Mouse state information:
//
//   bytes in the mouse_state var:
//      01234567
// byte 0: left mouse down
// byte 1: right mouse down
//
// each frame, mouse_state is shifted to
// the right, so the latter bits hold
// previous states.
//
// clicking is defined as letting go
// after having pressed a mouse button.

#define left_mouse_down         (mouse_state & 1<<7)
#define right_mouse_down        (mouse_state & 1<<6)
#define left_mouse_pressed      (!(mouse_state & 1<<7) && (mouse_state & 1<<5))
#define right_mouse_pressed     (!(mouse_state & 1<<6) && (mouse_state & 1<<4))

void init_input() {
    mouse_x = 0;
    mouse_y = 0;
    mouse_state = 0;
}

void update_input() {
    glfwGetCursorPos(window, &mouse_x, &mouse_y);

    i32 left_mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT),
        right_mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

    mouse_state >>= 2;
    if(left_mouse_button) {
        mouse_state |= (1<<7);
    }
    else {
        mouse_state &= ~(1<<7);
    }

    if(right_mouse_button) {
        mouse_state |= (1<<6);
    }
    else {
        mouse_state &= ~(1<<6);
    }
}
