// Program Options
#define                     FPS 60.0
#define           FIELD_OF_VIEW 85.0
#define                   MAP_W 100
#define                   MAP_H 100
#define            RESOURCE_DIR "./resource/"
#define              SHADER_DIR "shader/"
#define              NOISE_SEED 123456
//

#include "gl_load.cpp"

// External Libraries
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//

// Internal Libraries
#include "ext/rf_utils.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "ext/HandmadeMath.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "ext/stb_image.h"
//

// Game Code
#include "globals.cpp"
#include "resource.cpp"
#include "draw.cpp"
#include "state.cpp"
//

int main() {
    if(glfwInit()) {
        glfwWindowHint(GLFW_RESIZABLE, 1);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

        window_w = 1600;
        window_h = 900;
        window = glfwCreateWindow(window_w, window_h, "Plane Crawler", 0, 0);
        if(window) {
            { // center window
                const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                glfwSetWindowPos(window, mode->width/2 - window_w/2, mode->height/2 - window_h/2);
            }

            glfwMakeContextCurrent(window);
            glfwSwapInterval(0);

            if(ogl_LoadFunctions()) {
                init_draw();

                srand((unsigned int)time(NULL));

                state = init_title();
                next_state.type = 0;

                while(!glfwWindowShouldClose(window)) {
                    current_time = glfwGetTime();

                    glfwPollEvents();
                    glfwGetWindowSize(window, &window_w, &window_h);
                    glfwGetCursorPos(window, &mouse_x, &mouse_y);

                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    glClearColor(0, 0, 0, 1);
                    glViewport(0, 0, window_w, window_h);
                    { // @Update
                        projection = HMM_Perspective(FIELD_OF_VIEW, (r32)window_w/window_h, 1.f, 1000.f);
                        update_state();
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
