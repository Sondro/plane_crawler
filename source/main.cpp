//-------------------------------------------------------
//
// Source Notes
//
//-------------------------------------------------------
//
// Comments beginning with @ are located at
// notable places in the source code that might
// need to be often revisited or modified in
// feature addition.
//
// Note comments begin with NOTE
// Todo comments begin with TODO
//
//-------------------------------------------------------
//
// Variables/functions are lower_case_with_underscores
//
// Typenames except for primitive types and math types
// are UpperCamelCase
//
// Enum values are prefixed with their type in all caps,
// and are then in lowercase (ENUM_value)
//
// Constants are UPPER_CASE_WITH_UNDERSCORES
//
// Macro functions are named in the same fashion as
// actual functions
//
//-------------------------------------------------------
//
// For fixed-size and floating-point types, there are
// some typedefs that are used:
//
// * int<n>_t is typedef'd as i<n>, where n (without 
//   the angle brackets) is a number of bits. For 
//   example, i32
//
// * uint<n>_t is typedef'd as u<n>, in similar fashion
//   as signed integers
//
// * r32 for float
// * r64 for double
//
// This is all provided by "ext/rf_utils.h", which also
// has other stuff that is commonly used (like foreach
// loops or forrng loops). Look in that file to get more
// thorough information
//
//-------------------------------------------------------

// Program Options
#define DEBUG

#define WINDOW_TITLE      "Plane Crawler"
#define DEFAULT_WINDOW_W  1280
#define DEFAULT_WINDOW_H  720

#ifndef DEBUG
#error "Release version has not been prepared; you must #define DEBUG"
#endif

// External Libraries/Related Code
#include "gl_load.cpp"
#include <AL/al.h>
#include <AL/alc.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

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

#define DR_WAV_IMPLEMENTATION
#include "ext/dr_wav.h"

#include "ext/stb_vorbis.c"
#undef R
#undef L
#undef C
// NOTE(Ryan): stb_vorbis keeps R, L, and C defined
//             for whatever reason... this hurts when
//             we do something like some_vector.R

// Game Code
#include "globals.cpp"
#include "noise.cpp"
#include "input.cpp"
#include "assets.cpp"
#include "audio.cpp"
#include "draw.cpp"
#include "ui.cpp"
#include "state.cpp"

int main() {
    if(glfwInit()) {
        glfwWindowHint(GLFW_RESIZABLE, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        
        window_w = DEFAULT_WINDOW_W;
        window_h = DEFAULT_WINDOW_H;
        window = glfwCreateWindow(window_w, window_h, WINDOW_TITLE, 0, 0);
        if(window) {
            { // NOTE(Ryan): This centers the window...
                const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                glfwSetWindowPos(window, mode->width/2 - window_w/2, mode->height/2 - window_h/2);
            }
            
            glfwMakeContextCurrent(window);
            glfwSwapInterval(vsync ? 1 : 0);
            
            if(ogl_LoadFunctions()) {
                init_input();
                init_assets();
                init_draw();
                init_ui();
                
                srand((unsigned int)time(NULL));
                
                state = init_title_state();
                next_state.type = 0;
                
                i8 last_fullscreen;
                
                
                // NOTE(Ryan):
                //
                // Resources that will pretty much always
                // need to be loaded:
                request_texture(TEX_font);
                request_shader(SHADER_texture);
                request_shader(SHADER_rect);
                
                // we'll call update_assets once because
                // we want to load all of the defaultly loaded
                // stuff
                update_assets();
                
                while(!glfwWindowShouldClose(window)) {
                    last_time = current_time;
                    current_time = glfwGetTime();
                    
                    
                    // NOTE(Ryan): 
                    // 
                    // If the game is running slower than 10 FPS, bad
                    // things can happen, so we prevent delta_t from
                    // being larger than that.
                    if(delta_t > 1/10.f) {
                        last_time = current_time - 1/10.f;
                    }
                    
                    last_fullscreen = fullscreen;
                    
                    update_input();
                    update_audio();
                    
                    { // @Game Update
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                        glClearColor(0, 0, 0, 1);
                        glViewport(0, 0, window_w, window_h);
                        
                        ui_begin();
                        update_state();
                        ui_end();
                        draw_ui_filled_rect(v4(0, 0, 0, state_t < 0.95 ? state_t : 1), v4(0, 0, window_w, window_h));
                        glfwSwapBuffers(window);
                    }
                    
                    { // OpenGL Error Checking
                        GLenum err = glGetError();
                        if(err) {
                            fprintf(log_file, "ERROR [OpenGL]: %i\n\n", err);
                        }
                    }
                    
                    { // OpenAL Error Checking
                        ALenum err = alGetError();
                        if(err) {
                            fprintf(log_file, "ERROR [OpenAL]: %i\n\n", err);
                        }
                    }
                    
                    // State changes or updates
                    if(next_state.type || need_asset_refresh) {
                        state_t += (1-state_t) * 8 * delta_t;
                        if(state_t >= 0.99) {
                            state_t = 1;
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
                        state_t -= state_t * 4 * delta_t;
                    }
                    
                    // Fullscreen toggle
                    if(!keyboard_used && key_pressed[KEY_F11]) {
                        fullscreen = !fullscreen;
                    }
                    
                    // Fullscreen change (if necessary)
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
                fprintf(log_file, "ERROR: OpenGL loading failed - terminating...\n\n");
            }
            
            glfwDestroyWindow(window);
        }
        else {
            fprintf(log_file, "ERROR: GLFW window creation failed - terminating...\n\n");
        }
        glfwTerminate();
    }
    else {
        fprintf(log_file, "ERROR: GLFW initialization failed - terminating...\n\n");
    }
    
    clean_up_audio();
    
    return 0;
}
