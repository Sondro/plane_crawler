// Program Options
#define                     FPS 60.0
#define           FIELD_OF_VIEW 85.0
#define                   MAP_W 100
#define                   MAP_H 100
#define            RESOURCE_DIR "./resource/"
#define              SHADER_DIR "shader/"
#define              NOISE_SEED 123456
#define           MAX_UI_RENDER 256
#define MAX_UI_RENDER_TEXT_SIZE 32
//

// External Libraries/Related Code
#include "gl_load.cpp"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
//

// Internal Libraries
#include "ext/rf_utils.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "ext/HandmadeMath.h"
typedef hmm_v2 v2;
typedef hmm_v3 v3;
typedef hmm_v4 v4;
typedef hmm_m4 m4;
#define v2(x, y)        HMM_Vec2(x, y)
#define v3(x, y, z)     HMM_Vec3(x, y, z)
#define v4(x ,y, z, w)  HMM_Vec4(x, y, z, w)
#define m4(d)           HMM_Mat4d(d)

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "ext/stb_image.h"
//

// Game Code
#include "globals.cpp"
#include "input.cpp"
#include "resource.cpp"
#include "draw.cpp"
#include "ui.cpp"
#include "state.cpp"
//

int main() {
    if(glfwInit()) {
        glfwWindowHint(GLFW_RESIZABLE, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        window_w = 1024;
        window_h = 600;
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
                init_draw();
                init_ui();   

                srand((unsigned int)time(NULL));

                state = init_title();
                next_state.type = 0;

                while(!glfwWindowShouldClose(window)) {
                    current_time = glfwGetTime();

                    glfwPollEvents();
                    glfwGetWindowSize(window, &window_w, &window_h);
                    update_input();

                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    glClearColor(0, 0, 0, 1);
                    glViewport(0, 0, window_w, window_h);
                    { // @Update
                        projection = HMM_Perspective(FIELD_OF_VIEW, (r32)window_w/window_h, 1.f, 1000.f);
                        
                        ui_begin();
                        update_state();
                        ui_end();
                    }
                    glfwSwapBuffers(window);

                    {
                        GLenum err = glGetError();
                        if(err) {
                            fprintf(log_file, "ERROR [OpenGL]: %i\n\n", err);
                        }
                    }

                    // @State change
                    if(next_state.type) {
                        clean_up_state();
                        state.mem = next_state.mem;
                        state.type = next_state.type;
                        next_state.mem = 0;
                        next_state.type = 0;
                    }

                    while(glfwGetTime() < current_time + (1.0 / FPS));
                }

                clean_up_state();
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
