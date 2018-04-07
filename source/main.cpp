/*

TO-DO:

 * Implement real spell casting
 * Stop calling glGetUniformLocation every frame (cache results once)
 * Make real room generation
 * Implement gamepad controls
 * Audio

*/

// Program Options
#define DEBUG

#define ASSETS_DIR "./assets/"
#define SHADER_DIR "shader/"
#define TEXURE_DIR "./"

#define NOISE_SEED 123456

#define                 MAP_W 40
#define                 MAP_H 40
#define       MAX_ENEMY_COUNT 256
#define    MAX_PARTICLE_COUNT 4096
#define  MAX_PROJECTILE_COUNT 256
//

// External Libraries/Related Code
#include "gl_load.cpp"
#include <AL/al.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
//

// Internal Libraries
#include "ext/rf_utils.h"

#define RF_DARRAY_IMPLEMENTATION
#define RF_DARRAY_SHORT_NAMES
#include "ext/rf_darray.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "ext/HandmadeMath.h"
#include "hmm_wrapper.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "ext/stb_image.h"
//

// Game Code
#include "globals.cpp"
#include "audio.cpp"
#include "noise.cpp"
#include "input.cpp"

#ifdef DEBUG
#include "assets_loose.cpp"
#else
#error "Release version has not been prepared; you must #define DEBUG"
#endif

#include "draw.cpp"
#include "ui.cpp"
#include "state.cpp"
//

int main() {
    if(glfwInit()) {
        glfwWindowHint(GLFW_RESIZABLE, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        window_w = 1280;
        window_h = 768;
        window = glfwCreateWindow(window_w, window_h, "Plane Crawler", 0, 0);
        if(window) {
            { // @Note (Ryan) This centers the window...
                const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                glfwSetWindowPos(window, mode->width/2 - window_w/2, mode->height/2 - window_h/2);
            }

            glfwMakeContextCurrent(window);
            glfwSwapInterval(0);

            if(ogl_LoadFunctions()) {
                init_input();
                init_assets();
                init_draw();
                init_ui();

                srand((unsigned int)time(NULL));

                state = init_title();
                next_state.type = 0;

                i8 last_fullscreen;
                
                //
                // @Note (Ryan)
                //
                // Resources that will pretty much always
                // need to be loaded:
                //
                request_texture(TEX_font);
                request_shader(SHADER_texture);
                request_shader(SHADER_rect);

                update_assets();

                while(!glfwWindowShouldClose(window)) {
                    last_time = current_time;
                    current_time = glfwGetTime();
                    if(delta_t > 1/10.f) {
                        last_time = current_time - 1/10.f;
                    }

                    last_fullscreen = fullscreen;

                    last_key = 0;
                    last_char = 0;
                    glfwPollEvents();
                    glfwGetWindowSize(window, &window_w, &window_h);
                    update_input();

                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    glClearColor(0, 0, 0, 1);
                    glViewport(0, 0, window_w, window_h);
                    { // @Update
                        ui_begin();
                        update_state();
                        ui_end();
                        draw_ui_filled_rect(v4(0, 0, 0, state_t < 0.95 ? state_t : 1), v4(0, 0, window_w, window_h));
                    }
                    glfwSwapBuffers(window);

                    {
                        GLenum err = glGetError();
                        if(err) {
                            fprintf(log_file, "ERROR [OpenGL]: %i\n\n", err);
                        }
                    }

                    // @State changes or updates
                    if(next_state.type || need_asset_refresh) {
                        state_t += (1-state_t) * 12 * delta_t;
                        if(state_t >= 0.98) {
                            update_assets();
                            need_asset_refresh = 0;
                            if(next_state.type) {
                                clean_up_state();
                                state.mem = next_state.mem;
                                state.type = next_state.type;
                                next_state.mem = 0;
                                next_state.type = 0;
                                first_state_frame = 1;
                            }
                        }
                    }
                    else {
                        state_t -= state_t * 12 * delta_t;
                    }

                    if(!keyboard_used && key_pressed[KEY_F11]) {
                        fullscreen = !fullscreen;
                    }

                    if(fullscreen != last_fullscreen) {
                        GLFWmonitor *monitor = glfwGetWindowMonitor(window) ? NULL : glfwGetPrimaryMonitor();
                        glfwWindowHint(GLFW_RESIZABLE, 1);
                        glfwSetWindowMonitor(window, monitor, 0, 0, window_w, window_h, GLFW_DONT_CARE);
                    }

                    if(fps <= 359.f) {
                        while(glfwGetTime() < current_time + (1.0 / fps));
                    }
                }
                
                clean_up_state();
                clean_up_assets();
                clean_up_draw();
            }
            else {
                fprintf(log_file, "ERROR: OpenGL loading failed\n\n");
            }

            glfwDestroyWindow(window);
        }
        else {
            fprintf(log_file, "ERROR: GLFW window creation failed\n\n");
        }
        glfwTerminate();
    }
    else {
        fprintf(log_file, "ERROR: GLFW initialization failed\n\n");
    }

    return 0;
}
