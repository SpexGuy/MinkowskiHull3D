#include <iostream>
#include <cstdlib>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <stb/stb_image.h>
#include "gl_includes.h"
#include "Perf.h"
#include "hull3D.h"

using namespace std;
using namespace glm;

GLuint compileShader(const char *vertSrc, const char *fragSrc);
void loadTexture(GLuint texname, const char *filename);

GLFWwindow *window;

struct {
    GLuint normalMat;
    GLuint mvp;
} uniforms;

const char *vert = GLSL(
    const vec3 lightDir = normalize(vec3(1,1,1));
    const float ambient = 0.2;

    uniform mat3 normalMat;
    uniform mat4 mvp;

    in vec3 position;
    in vec3 normal;
    in vec3 color;

    flat out vec3 litColor;

    void main() {
        // lighting is not realistic. Meant just to distinguish the faces.
        gl_Position = mvp * vec4(position, 1.0);
        vec3 normal = normalMat * normal;
        float light = dot(normal, lightDir) / 2 + 0.5;
        light = light + ambient * (1 - light);
        litColor = light * color;
    }
);

const char *frag = GLSL(

    flat in vec3 litColor;
    out vec4 fragColor;

    void main() {
        fragColor = vec4(litColor, 1);
    }
);

mat4 rotation; // identity
mat4 view;
mat4 projection;

int animationStepsLeft = 0;
int animationFramesLeft = 0;
int animationFrameReset = 30;

SphereCollider3D collider;

SurfaceState state;

int numVerts;

struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 color;
};

void dumpState() {
    printf("Points:\n");
    for (int c = 0, n = state.points.size(); c < n; c++) {
        vec3 &pt = state.points[c];
        printf("  %2d: (%f, %f, %f)\n", c, pt.x, pt.y, pt.z);
    }

    printf("\nTriangles:\n");
    for (int c = 0, n = state.triangles.size(); c < n; c++) {
        Triangle &tri = state.triangles[c];
        printf("  %2d: (%d %d %d), by (%d:%d, %d:%d, %d:%d)\n",
            c, tri.edges[0].vertex, tri.edges[1].vertex, tri.edges[2].vertex,
               tri.edges[0].opposite / 4, tri.edges[0].opposite & 3,
               tri.edges[1].opposite / 4, tri.edges[1].opposite & 3,
               tri.edges[2].opposite / 4, tri.edges[2].opposite & 3);
    }

    printf("\nCurrent: %d\n\n", state.current);
}

const vec3 kOutsideColor = vec3(1, 0.5, 0);
const vec3 kInsideColor = vec3(0.5, 0, 1);
const vec3 kNextColor = vec3(0, 1, 0.5);

void updateMesh() {
    vector<Vertex> verts;
    verts.reserve(state.triangles.size() * 3); // 3 verts per triangle.
//    int tri0 = -1, tri1 = -1, tri2 = -1;
//    if (!state.done()) {
//        tri0 = state.triangles[state.current].edges[0].opposite / 4;
//        tri1 = state.triangles[state.current].edges[1].opposite / 4;
//        tri2 = state.triangles[state.current].edges[2].opposite / 4;
//    }
    for (int i = 0, n = state.triangles.size(); i < n; i++) {
        Triangle &tri = state.triangles[i];
        vec3 a = state.points[tri.edges[0].vertex];
        vec3 b = state.points[tri.edges[1].vertex];
        vec3 c = state.points[tri.edges[2].vertex];
        vec3 normal = normalize(cross(c - b, a - b));

        Vertex vert;
        vert.position = a;
        vert.normal = normal;
        vert.color = i < state.current ? kOutsideColor : i == state.current ? kNextColor : kInsideColor;
//        if (i == tri0 || i == tri1 || i == tri2) vert.color = vec3(1,0,0);
        verts.push_back(vert);
        vert.position = b;
        verts.push_back(vert);
        vert.position = c;
        verts.push_back(vert);

    }
    numVerts = verts.size();
    glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(Vertex), verts.data(), GL_DYNAMIC_DRAW);
    checkError();
}

void setup() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    //glDisable(GL_CULL_FACE);
    checkError();

    GLuint shader = compileShader(vert, frag);
    glUseProgram(shader);
    uniforms.mvp = glGetUniformLocation(shader, "mvp");
    uniforms.normalMat = glGetUniformLocation(shader, "normalMat");
    checkError();

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    checkError();

    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    checkError();

    GLuint pos = glGetAttribLocation(shader, "position");
    GLuint nor = glGetAttribLocation(shader, "normal");
    GLuint col = glGetAttribLocation(shader, "color");
    glEnableVertexAttribArray(pos);
    glEnableVertexAttribArray(nor);
    glEnableVertexAttribArray(col);
    glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(nor, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) sizeof(vec3));
    glVertexAttribPointer(col, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (sizeof(vec3) * 2));
    checkError();

    view = lookAt(vec3(0, 0, 5), vec3(0), vec3(0, 1, 0));
    rotation = lookAt(vec3(0), vec3(-1), vec3(0, 1, 0));

    collider.radius = 2;
    state.object = &collider;
    state.epsilon = 0.05;
    state.init();
    updateMesh();
}

void draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, numVerts);
}

void updateMatrices() {
    mat4 mvp = projection * view * rotation;
    checkError();
    glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, &mvp[0][0]);
    mat3 normal = mat3(view * rotation);
    glUniformMatrix3fv(uniforms.normalMat, 1, GL_FALSE, &normal[0][0]);
}

static void glfw_resize_callback(GLFWwindow *window, int width, int height) {
    printf("resize: %dx%d\n", width, height);
    glViewport(0, 0, width, height);
    if (height != 0) {
        float aspect = float(width) / height;
        projection = perspective(62.f, aspect, 0.5f, 10.f);
        updateMatrices();
    }
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    } else if (key == GLFW_KEY_W) {
        static bool wireframe = false;
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    } else if (key == GLFW_KEY_S) {
        if (mods & GLFW_MOD_SHIFT) {
            animationStepsLeft = 10;
        } else {
            state.step();
            updateMesh();
        }
    } else if (key == GLFW_KEY_D) {
        dumpState();
    }
}

vec2 lastMouse = vec2(-1,-1);

static void glfw_click_callback(GLFWwindow *window, int button, int action, int mods) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) lastMouse = vec2(x, y);
        else if (action == GLFW_RELEASE) lastMouse = vec2(-1, -1);
    }
}

static void glfw_mouse_callback(GLFWwindow *window, double xPos, double yPos) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS) return;
    if (lastMouse == vec2(-1,-1)) {
        lastMouse = vec2(xPos, yPos);
        return; // can't update this frame, no previous data.
    } else {
        vec2 current = vec2(xPos, yPos);
        vec2 delta = current - lastMouse;
        if (delta == vec2(0,0)) return;

        vec3 rotationVector = vec3(delta.y, delta.x, 0);
        float angle = length(delta);
        rotation = rotate(angle, rotationVector) * rotation;
        updateMatrices();

        lastMouse = current;
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

GLuint compileShader(const char *vertSrc, const char *fragSrc) {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertSrc, nullptr);
    glCompileShader(vertex);
    checkShaderError(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragSrc, nullptr);
    glCompileShader(fragment);
    checkShaderError(fragment);

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vertex);
    glAttachShader(shader, fragment);
    glLinkProgram(shader);
    checkLinkError(shader);

    return shader;
}

void loadTexture(GLuint texname, const char *filename) {
    glBindTexture(GL_TEXTURE_2D, texname);

    int width, height, bpp;
    unsigned char *pixels = stbi_load(filename, &width, &height, &bpp, STBI_default);
    if (pixels == nullptr) {
        cout << "Failed to load image " << filename << " (" << stbi_failure_reason() << ")" << endl;
        return;
    }
    cout << "Loaded " << filename << ", " << height << 'x' << width << ", comp = " << bpp << endl;

    GLenum format;
    switch(bpp) {
        case STBI_rgb:
            format = GL_RGB;
            break;
        case STBI_rgb_alpha:
            format = GL_RGBA;
            break;
        default:
            cout << "Unsupported format: " << bpp << endl;
            return;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(pixels);
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
#ifdef APPLE
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    window = glfwCreateWindow(640, 480, "Minkowski Hull 3D", NULL, NULL);
    if (!window) {
        cout << "Failed to create window" << endl;
        exit(-1);
    }

    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_click_callback);
    glfwSetCursorPosCallback(window, glfw_mouse_callback);

    glfwMakeContextCurrent(window);

    // If the program is crashing at glGenVertexArrays, try uncommenting this line.
    //glewExperimental = GL_TRUE;
    glewInit();

    printf("OpenGL version recieved: %s\n", glGetString(GL_VERSION));

    glfwSwapInterval(1);

    initPerformanceData();

    setup();
    checkError();

    glfwSetFramebufferSizeCallback(window, glfw_resize_callback); // do this after setup
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glfw_resize_callback(window, width, height); // call resize once with the initial size

    // make sure performance data is clean going into main loop
    markPerformanceFrame();
    printPerformanceData();
    double lastPerfPrintTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {

        {
            Perf stat("Poll events");
            glfwPollEvents();
            checkError();
        }
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

        markPerformanceFrame();

        double now = glfwGetTime();
        if (now - lastPerfPrintTime > 10.0) {
            printPerformanceData();
            lastPerfPrintTime = now;
        }
    }

    return 0;
}
