#define STB_IMAGE_IMPLEMENTATION
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include "render.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

GLFWwindow* window;

#define check_error(handle, get, attr, log, msg) { \
    GLint success;                                  \
    get(handle, attr, &success);                     \
    if (!success) {                                   \
        char infolog[512];                             \
        log(handle, 512, NULL, infolog);                \
        fprintf(stderr, "%s:\n%s\n", msg, infolog);      \
    }                                                     \
}

#define check_compile_error(handle) check_error(handle, glGetShaderiv,  GL_COMPILE_STATUS, glGetShaderInfoLog,  "Shader failed to compile")
#define check_link_error(   handle) check_error(handle, glGetProgramiv, GL_LINK_STATUS,    glGetProgramInfoLog, "Shader failed to link")

static char* read_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* data = malloc(size + 1);
    fread(data, size, 1, f);
    data[size] = 0;
    fclose(f);
    return data;
}

static GLuint shader, vao, vbo;

static float vertices[] = {
    -1, -1,
    +1, -1,
    -1, +1,
    +1, +1,
};

static int indices[] = {
    0, 1, 2,
    2, 3, 0,
};

static void init_shader(const char* fragment_file, const char* vertex_file) {
    char* shader_vertex = read_file(vertex_file);
    char* shader_fragment = read_file(fragment_file);
    
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, (const char**)&shader_vertex, NULL);
    glCompileShader(vertex);
    check_compile_error(vertex);
    
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, (const char**)&shader_fragment, NULL);
    glCompileShader(fragment);
    check_compile_error(fragment);
    
    shader = glCreateProgram();
    glAttachShader(shader, vertex);
    glAttachShader(shader, fragment);
    glLinkProgram(shader);
    check_link_error(shader);
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    free(shader_vertex);
    free(shader_fragment);

    glUseProgram(shader);
}

static void init_buffers() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float[2]), (void*)0);
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void render_init(const char* fragment, const char* vertex) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    int width, height;
    window = glfwCreateWindow(384*4, 256*4, "shader test", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    init_shader(fragment, vertex);
    init_buffers();
}

void render_update(void(*func)(double dt)) {
    static double start;
    double curr = glfwGetTime();
    double dt = curr - start;
    start = curr;
    glfwPollEvents();
    func(dt);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
}

bool render_quit() {
    bool should_close = glfwWindowShouldClose(window);
    if (!should_close) return false;
    glfwDestroyWindow(window);
    glfwTerminate();
    return true;
}

uint render_bind_buffer(uint binding) {
    uint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, buffer);
    return buffer;
}

uint render_bind_texture(const char* filename) {
    int w, h, c;
    unsigned char* data = stbi_load(filename, &w, &h, &c, 4);
    uint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return texture;
}

void render_use_texture(uint texture) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
}

void render_upload_int(const char* name, int value) {
    glUniform1i(glGetUniformLocation(shader, name), value);
}

void render_upload_float(const char* name, float value) {
    glUniform1f(glGetUniformLocation(shader, name), value);
}

void render_upload_float2(const char* name, void* values) {
    glUniform2fv(glGetUniformLocation(shader, name), 1, values);
}

void render_upload_float3(const char* name, void* values) {
    glUniform3fv(glGetUniformLocation(shader, name), 1, values);
}

void render_upload_float4(const char* name, void* values) {
    glUniform4fv(glGetUniformLocation(shader, name), 1, values);
}

void render_upload_buffer(uint buffer, size_t size, void* data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
}
