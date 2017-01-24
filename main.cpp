#include <iostream>
#include <cstdlib>

#include <cmath>

#include <glm/glm.hpp>
#include "gl_includes.h"
#include "Perf.h"

using namespace std;
using namespace glm;

void setup() {
    // TODO
    // init shaders and other global state
}

void draw() {
    // TODO
    // draw things
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    } else if (key == GLFW_KEY_W) {
        static bool wireframe = false;
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    }
}

void glfw_error_callback(int error, const char* description) {
    cerr << "GLFW Error: " << description << " (error " << error << ")" << endl;
}

void checkShaderError(GLuint shader) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success) return;

    cout << "Shader Compile Failed." << endl;

    GLint logSize = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize == 0) {
        cout << "No log found." << endl;
        return;
    }

    GLchar *log = new GLchar[logSize];

    glGetShaderInfoLog(shader, logSize, &logSize, log);

    cout << log << endl;

    delete[] log;
}

void checkLinkError(GLuint program) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success) return;

    cout << "Shader link failed." << endl;

    GLint logSize = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize == 0) {
        cout << "No log found." << endl;
        return;
    }

    GLchar *log = new GLchar[logSize];

    glGetProgramInfoLog(program, logSize, &logSize, log);
    cout << log << endl;

    delete[] log;
}

int main() {
    if (!glfwInit()) {
        cout << "Failed to init GLFW" << endl;
        exit(-1);
    }
    cout << "GLFW Successfully Started" << endl;

    glfwSetErrorCallback(glfw_error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(640, 480, "Stupid SH Tricks", NULL, NULL);
    if (!window) {
        cout << "Failed to create window" << endl;
        exit(-1);
    }
    glfwSetKeyCallback(window, glfw_key_callback);

    glfwMakeContextCurrent(window);
    glewInit();

    glfwSwapInterval(1);

    initPerformanceData();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    setup();

    // make sure performance data is clean going into main loop
    markPerformanceFrame();
    printPerformanceData();
    double lastPerfPrintTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {

        {
            Perf stat("Draw");
            draw();
            checkError();
        }

        {
            Perf stat("Swap buffers");
            glfwSwapBuffers(window);
            checkError();
        }
        {
            Perf stat("Poll events");
            glfwPollEvents();
            checkError();
        }

        double now = glfwGetTime();
        if (now - lastPerfPrintTime > 10.0) {
            printPerformanceData();
            lastPerfPrintTime = now;
        }
    }

    return 0;
}
