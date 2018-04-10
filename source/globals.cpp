#define delta_t (current_time - last_time)

global FILE *log_file = stderr;
global GLFWwindow *window = 0;
global i32 window_w, window_h;
global r64 current_time = 0, last_time = 0;

// @Note (Ryan) global settings
global i8 fullscreen = 0, vsync = 0;
global r32 field_of_view = 110.f;
global r32 fps = 60.f;

r32 random32(r32 low, r32 high) {
    return low + (((r32)((unsigned int)rand() % 1000) / 1000.f) * (high - low));
}

i8 file_exists(const char *filename) {
    FILE *f = fopen(filename, "r");
    if(f) {
        fclose(f);
        return 1;
    }
    return 0;
}
