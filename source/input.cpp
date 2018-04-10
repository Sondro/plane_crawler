// Mouse
global r64 mouse_x, mouse_y;
global i8 mouse_state;
global i8 mouse_pos_used = 0,
          mouse_buttons_used = 0;

// Keyboard
global i8 key_down[GLFW_KEY_LAST] = { 0 },
          key_pressed[GLFW_KEY_LAST] = { 0 };
global u8 last_char = 0;
global i16 last_key = 0, last_char_mods = 0;
global i8 keyboard_used = 0;

// Gamepad
global i32 gamepad_axis_count = 0, gamepad_button_count = 0;
global const r32 *gamepad_axes = NULL;
global const u8 *gamepad_button_states = NULL;
global u8 last_gamepad_button_states[16];

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

#define left_mouse_down         (!mouse_buttons_used && mouse_state & 1<<7)
#define right_mouse_down        (!mouse_buttons_used && mouse_state & 1<<6)
#define left_mouse_pressed      (!mouse_buttons_used && !(mouse_state & 1<<7) && (mouse_state & 1<<5))
#define right_mouse_pressed     (!mouse_buttons_used && !(mouse_state & 1<<6) && (mouse_state & 1<<4))

#define key_control_down(i)     (!keyboard_used && key_down[key_control_maps[i]])
#define key_control_pressed(i)  (!keyboard_used && key_pressed[key_control_maps[i]])

#define joystick_1_x        (gamepad_axes ? gamepad_axes[0] : 0)
#define joystick_1_y        (gamepad_axes && gamepad_axis_count > 1 ? gamepad_axes[1] : 0)
#define joystick_2_x        (gamepad_axes && gamepad_axis_count > 2 ? gamepad_axes[2] : 0)
#define joystick_2_y        (gamepad_axes && gamepad_axis_count > 3 ? gamepad_axes[3] : 0)

#define gamepad_button_down(i)      (gamepad_button_count > i && gamepad_button_states ? gamepad_button_states[i] == GLFW_PRESS : 0)
#define gamepad_button_pressed(i)   (gamepad_button_count > i && gamepad_button_states ? \
                                     gamepad_button_states[i] == GLFW_PRESS && last_gamepad_button_states[i] != GLFW_PRESS : 0)

#define gamepad_control_down(i)    (gamepad_button_down(gamepad_control_maps[i]))
#define gamepad_control_pressed(i) (gamepad_button_pressed(gamepad_control_maps[i]))

enum {
    KEY_SPACE               = GLFW_KEY_SPACE,
    KEY_APOSTROPHE          = GLFW_KEY_APOSTROPHE,
    KEY_COMMA               = GLFW_KEY_COMMA,
    KEY_MINUS               = GLFW_KEY_MINUS,
    KEY_PERIOD              = GLFW_KEY_PERIOD,
    KEY_SLASH               = GLFW_KEY_SLASH,
    KEY_0                   = GLFW_KEY_0,
    KEY_1                   = GLFW_KEY_1,
    KEY_2                   = GLFW_KEY_2,
    KEY_3                   = GLFW_KEY_3,
    KEY_4                   = GLFW_KEY_4,
    KEY_5                   = GLFW_KEY_5,
    KEY_6                   = GLFW_KEY_6,
    KEY_7                   = GLFW_KEY_7,
    KEY_8                   = GLFW_KEY_8,
    KEY_9                   = GLFW_KEY_9,
    KEY_SEMICOLON           = GLFW_KEY_SEMICOLON,
    KEY_EQUAL               = GLFW_KEY_EQUAL,
    KEY_A                   = GLFW_KEY_A,
    KEY_B                   = GLFW_KEY_B,
    KEY_C                   = GLFW_KEY_C,
    KEY_D                   = GLFW_KEY_D,
    KEY_E                   = GLFW_KEY_E,
    KEY_F                   = GLFW_KEY_F,
    KEY_G                   = GLFW_KEY_G,
    KEY_H                   = GLFW_KEY_H,
    KEY_I                   = GLFW_KEY_I,
    KEY_J                   = GLFW_KEY_J,
    KEY_K                   = GLFW_KEY_K,
    KEY_L                   = GLFW_KEY_L,
    KEY_M                   = GLFW_KEY_M,
    KEY_N                   = GLFW_KEY_N,
    KEY_O                   = GLFW_KEY_O,
    KEY_P                   = GLFW_KEY_P,
    KEY_Q                   = GLFW_KEY_Q,
    KEY_R                   = GLFW_KEY_R,
    KEY_S                   = GLFW_KEY_S,
    KEY_T                   = GLFW_KEY_T,
    KEY_U                   = GLFW_KEY_U,
    KEY_V                   = GLFW_KEY_V,
    KEY_W                   = GLFW_KEY_W,
    KEY_X                   = GLFW_KEY_X,
    KEY_Y                   = GLFW_KEY_Y,
    KEY_Z                   = GLFW_KEY_Z,
    KEY_LEFT_BRACKET        = GLFW_KEY_LEFT_BRACKET,
    KEY_BACKSLASH           = GLFW_KEY_BACKSLASH,
    KEY_RIGHT_BRACKET       = GLFW_KEY_RIGHT_BRACKET,
    KEY_GRAVE_ACCENT        = GLFW_KEY_GRAVE_ACCENT,
    KEY_WORLD_1             = GLFW_KEY_WORLD_1,
    KEY_WORLD_2             = GLFW_KEY_WORLD_2,
    KEY_ESCAPE              = GLFW_KEY_ESCAPE,
    KEY_ENTER               = GLFW_KEY_ENTER,
    KEY_TAB                 = GLFW_KEY_TAB,
    KEY_BACKSPACE           = GLFW_KEY_BACKSPACE,
    KEY_INSERT              = GLFW_KEY_INSERT,
    KEY_DELETE              = GLFW_KEY_DELETE,
    KEY_RIGHT               = GLFW_KEY_RIGHT,
    KEY_LEFT                = GLFW_KEY_LEFT,
    KEY_UP                  = GLFW_KEY_UP,
    KEY_DOWN                = GLFW_KEY_DOWN,
    KEY_PAGE_UP             = GLFW_KEY_PAGE_UP,
    KEY_PAGE_DOWN           = GLFW_KEY_PAGE_DOWN,
    KEY_HOME                = GLFW_KEY_HOME,
    KEY_END                 = GLFW_KEY_END,
    KEY_CAPS_LOCK           = GLFW_KEY_CAPS_LOCK,
    KEY_SCROLL_LOCK         = GLFW_KEY_SCROLL_LOCK,
    KEY_NUM_LOCK            = GLFW_KEY_NUM_LOCK,
    KEY_PRINT_SCREEN        = GLFW_KEY_PRINT_SCREEN,
    KEY_PAUSE               = GLFW_KEY_PAUSE,
    KEY_F1                  = GLFW_KEY_F1,
    KEY_F2                  = GLFW_KEY_F2,
    KEY_F3                  = GLFW_KEY_F3,
    KEY_F4                  = GLFW_KEY_F4,
    KEY_F5                  = GLFW_KEY_F5,
    KEY_F6                  = GLFW_KEY_F6,
    KEY_F7                  = GLFW_KEY_F7,
    KEY_F8                  = GLFW_KEY_F8,
    KEY_F9                  = GLFW_KEY_F9,
    KEY_F10                 = GLFW_KEY_F10,
    KEY_F11                 = GLFW_KEY_F11,
    KEY_F12                 = GLFW_KEY_F12,
    KEY_F13                 = GLFW_KEY_F13,
    KEY_F14                 = GLFW_KEY_F14,
    KEY_F15                 = GLFW_KEY_F15,
    KEY_F16                 = GLFW_KEY_F16,
    KEY_F17                 = GLFW_KEY_F17,
    KEY_F18                 = GLFW_KEY_F18,
    KEY_F19                 = GLFW_KEY_F19,
    KEY_F20                 = GLFW_KEY_F20,
    KEY_F21                 = GLFW_KEY_F21,
    KEY_F22                 = GLFW_KEY_F22,
    KEY_F23                 = GLFW_KEY_F23,
    KEY_F24                 = GLFW_KEY_F24,
    KEY_F25                 = GLFW_KEY_F25,
    KEY_KP_0                = GLFW_KEY_KP_0,
    KEY_KP_1                = GLFW_KEY_KP_1,
    KEY_KP_2                = GLFW_KEY_KP_2,
    KEY_KP_3                = GLFW_KEY_KP_3,
    KEY_KP_4                = GLFW_KEY_KP_4,
    KEY_KP_5                = GLFW_KEY_KP_5,
    KEY_KP_6                = GLFW_KEY_KP_6,
    KEY_KP_7                = GLFW_KEY_KP_7,
    KEY_KP_8                = GLFW_KEY_KP_8,
    KEY_KP_9                = GLFW_KEY_KP_9,
    KEY_KP_DECIMAL          = GLFW_KEY_KP_DECIMAL,
    KEY_KP_DIVIDE           = GLFW_KEY_KP_DIVIDE,
    KEY_KP_MULTIPLY         = GLFW_KEY_KP_MULTIPLY,
    KEY_KP_SUBTRACT         = GLFW_KEY_KP_SUBTRACT,
    KEY_KP_ADD              = GLFW_KEY_KP_ADD,
    KEY_KP_ENTER            = GLFW_KEY_ENTER,
    KEY_KP_EQUAL            = GLFW_KEY_EQUAL,
    KEY_LEFT_SHIFT          = GLFW_KEY_LEFT_SHIFT,
    KEY_LEFT_CONTROL        = GLFW_KEY_LEFT_CONTROL,
    KEY_LEFT_ALT            = GLFW_KEY_LEFT_ALT,
    KEY_LEFT_SUPER          = GLFW_KEY_LEFT_SUPER,
    KEY_RIGHT_SHIFT         = GLFW_KEY_RIGHT_SHIFT,
    KEY_RIGHT_CONTROL       = GLFW_KEY_RIGHT_CONTROL,
    KEY_RIGHT_ALT           = GLFW_KEY_RIGHT_ALT,
    KEY_RIGHT_SUPER         = GLFW_KEY_RIGHT_SUPER,
    KEY_MENU                = GLFW_KEY_MENU
};

enum { // @Key Controls
    KC_MOVE_FORWARD,
    KC_MOVE_BACKWARD,
    KC_MOVE_LEFT,
    KC_MOVE_RIGHT,
    KC_TURN_LEFT,
    KC_TURN_RIGHT,
    KC_ATTACK,
    KC_PAUSE,
    MAX_KC
};

i16 key_control_maps[MAX_KC] = {
    KEY_W,
    KEY_S,
    KEY_A,
    KEY_D,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_SPACE,
    KEY_ESCAPE,
};

const char *key_control_names[MAX_KC] = {
    "Move Forward",
    "Move Backward",
    "Move Left",
    "Move Right",
    "Turn Left",
    "Turn Right",
    "Attack",
    "Pause",
};

enum { // @Gamepad Controls
    GC_MOVE_FORWARD,
    GC_MOVE_BACKWARD,
    GC_MOVE_LEFT,
    GC_MOVE_RIGHT,
    GC_TURN_LEFT,
    GC_TURN_RIGHT,
    GC_ATTACK,
    GC_PAUSE,
};

i16 gamepad_control_maps[MAX_KC] = {
    10,
    12,
    13,
    11,
    4,
    5,
    0,
    7,
};

const char *gamepad_control_names[MAX_KC] = {
    "Move Forward",
    "Move Backward",
    "Move Left",
    "Move Right",
    "Turn Left",
    "Turn Right",
    "Attack",
    "Pause",
};

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if((action == GLFW_PRESS && last_key != key) || action == GLFW_REPEAT) {
        last_key = key;
    }
    else {
        last_key = 0;
    }
}

static void charmods_callback(GLFWwindow *window, unsigned int code_point, int mods) {
    last_char = code_point;
    last_char_mods = mods;
}

void init_input() {
    mouse_x = 0;
    mouse_y = 0;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    mouse_state = 0;
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharModsCallback(window, charmods_callback);
}

void update_input() {
	last_key = 0;
	last_char = 0;
	glfwPollEvents();
	glfwGetWindowSize(window, &window_w, &window_h);
    glfwGetCursorPos(window, &mouse_x, &mouse_y);

    mouse_pos_used = 0;
    mouse_buttons_used = 0;
    keyboard_used = 0;

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

    for(i16 i = 0; i < GLFW_KEY_LAST; i++) {
        i32 key_state = glfwGetKey(window, i);
        if(key_state == GLFW_PRESS) {
            key_pressed[i] = !key_down[i];
            key_down[i] = 1;
        }
        else {
            key_down[i] = 0;
            key_pressed[i] = 0;
        }
    }

    for(i8 i = 0; i < 16 && i < gamepad_button_count; i++) {
        last_gamepad_button_states[i] = gamepad_button_states[i];
    }
    gamepad_axes = glfwGetJoystickAxes(0, &gamepad_axis_count);
    gamepad_button_states = glfwGetJoystickButtons(0, &gamepad_button_count);
}

const char *key_name(u16 key) {
    switch(key) {
        case KEY_SPACE: { return "Space"; }
        case KEY_APOSTROPHE: { return "'"; }
        case KEY_COMMA: { return ","; }
        case KEY_MINUS: { return "-"; }
        case KEY_PERIOD: { return "."; }
        case KEY_SLASH: { return "/"; }
        case KEY_0: { return "0"; }
        case KEY_1: { return "1"; }
        case KEY_2: { return "2"; }
        case KEY_3: { return "3"; }
        case KEY_4: { return "4"; }
        case KEY_5: { return "5"; }
        case KEY_6: { return "6"; }
        case KEY_7: { return "7"; }
        case KEY_8: { return "8"; }
        case KEY_9: { return "9"; }
        case KEY_SEMICOLON: { return ";"; }
        case KEY_EQUAL: { return "="; }
        case KEY_A: { return "A"; }
        case KEY_B: { return "B"; }
        case KEY_C: { return "C"; }
        case KEY_D: { return "D"; }
        case KEY_E: { return "E"; }
        case KEY_F: { return "F"; }
        case KEY_G: { return "G"; }
        case KEY_H: { return "H"; }
        case KEY_I: { return "I"; }
        case KEY_J: { return "J"; }
        case KEY_K: { return "K"; }
        case KEY_L: { return "L"; }
        case KEY_M: { return "M"; }
        case KEY_N: { return "N"; }
        case KEY_O: { return "O"; }
        case KEY_P: { return "P"; }
        case KEY_Q: { return "Q"; }
        case KEY_R: { return "R"; }
        case KEY_S: { return "S"; }
        case KEY_T: { return "T"; }
        case KEY_U: { return "U"; }
        case KEY_V: { return "V"; }
        case KEY_W: { return "W"; }
        case KEY_X: { return "X"; }
        case KEY_Y: { return "Y"; }
        case KEY_Z: { return "Z"; }
        case KEY_LEFT_BRACKET: { return "["; }
        case KEY_BACKSLASH: { return "\\"; }
        case KEY_RIGHT_BRACKET: { return "]"; }
        case KEY_GRAVE_ACCENT: { return "`"; }
        case KEY_WORLD_1: { return "World 1"; }
        case KEY_WORLD_2: { return "World 2"; }
        case KEY_ESCAPE: { return "Escape"; }
        case KEY_ENTER: { return "Enter"; }
        case KEY_TAB: { return "Tab"; }
        case KEY_BACKSPACE: { return "Backspace"; }
        case KEY_INSERT: { return "Insert"; }
        case KEY_DELETE: { return "Delete"; }
        case KEY_RIGHT: { return "Right"; }
        case KEY_LEFT: { return "Left"; }
        case KEY_UP: { return "Up"; }
        case KEY_DOWN: { return "Down"; }
        case KEY_PAGE_UP: { return "Page Up"; }
        case KEY_PAGE_DOWN: { return "Page Down"; }
        case KEY_HOME: { return "Home"; }
        case KEY_END: { return "End"; }
        case KEY_CAPS_LOCK: { return "Caps Lock"; }
        case KEY_SCROLL_LOCK: { return "Scroll Lock"; }
        case KEY_NUM_LOCK: { return "Num Lock"; }
        case KEY_PRINT_SCREEN: { return "Print Screen"; }
        case KEY_PAUSE: { return "Pause"; }
        case KEY_F1: { return "F1"; }
        case KEY_F2: { return "F2"; }
        case KEY_F3: { return "F3"; }
        case KEY_F4: { return "F4"; }
        case KEY_F5: { return "F5"; }
        case KEY_F6: { return "F6"; }
        case KEY_F7: { return "F7"; }
        case KEY_F8: { return "F8"; }
        case KEY_F9: { return "F9"; }
        case KEY_F10: { return "F10"; }
        case KEY_F11: { return "F11"; }
        case KEY_F12: { return "F12"; }
        case KEY_F13: { return "F13"; }
        case KEY_F14: { return "F14"; }
        case KEY_F15: { return "F15"; }
        case KEY_F16: { return "F16"; }
        case KEY_F17: { return "F17"; }
        case KEY_F18: { return "F18"; }
        case KEY_F19: { return "F19"; }
        case KEY_F20: { return "F20"; }
        case KEY_F21: { return "F21"; }
        case KEY_F22: { return "F22"; }
        case KEY_F23: { return "F23"; }
        case KEY_F24: { return "F24"; }
        case KEY_F25: { return "F25"; }
        case KEY_KP_0: { return "0"; }
        case KEY_KP_1: { return "1"; }
        case KEY_KP_2: { return "2"; }
        case KEY_KP_3: { return "3"; }
        case KEY_KP_4: { return "4"; }
        case KEY_KP_5: { return "5"; }
        case KEY_KP_6: { return "6"; }
        case KEY_KP_7: { return "7"; }
        case KEY_KP_8: { return "8"; }
        case KEY_KP_9: { return "9"; }
        case KEY_KP_DECIMAL: { return "Decimal"; }
        case KEY_KP_DIVIDE: { return "Divide"; }
        case KEY_KP_MULTIPLY: { return "Multiply"; }
        case KEY_KP_SUBTRACT: { return "Subtract"; }
        case KEY_KP_ADD: { return "Add"; }
        case KEY_LEFT_SHIFT: { return "Left Shift"; }
        case KEY_LEFT_CONTROL: { return "Left Control"; }
        case KEY_LEFT_ALT: { return "Left Alt"; }
        case KEY_LEFT_SUPER: { return "Left Super"; }
        case KEY_RIGHT_SHIFT: { return "Right Shift"; }
        case KEY_RIGHT_CONTROL: { return "Right Control"; }
        case KEY_RIGHT_ALT: { return "Right Alt"; }
        case KEY_RIGHT_SUPER: { return "Right Super"; }
        case KEY_MENU: { return "Menu"; }
        default: break;
    }
    return "";
}
