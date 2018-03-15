global FILE *log_file = stderr;
global GLFWwindow *window = 0;

global i32 window_w, window_h;
global r64 mouse_x, mouse_y;

global r64 current_time = 0;

r32 random32(r32 low, r32 high) {
    return low + (((r32)((unsigned int)rand() % 1000) / 1000.f) * (high - low));
}
