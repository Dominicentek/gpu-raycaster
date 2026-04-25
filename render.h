#ifndef RENDER_H
#define RENDER_H

#include <stddef.h>
#include <stdbool.h>

#include <GLFW/glfw3.h>

extern GLFWwindow* window;

typedef unsigned int uint;

void render_init(const char* fragment, const char* vertex);
void render_update(void(*func)(double delta_time));
bool render_quit();

uint render_bind_buffer(uint binding);
uint render_bind_texture(const char* filename);
void render_use_texture(uint texture);
void render_upload_int(const char* name, int value);
void render_upload_float(const char* name, float value);
void render_upload_float2(const char* name, void* values);
void render_upload_float3(const char* name, void* values);
void render_upload_float4(const char* name, void* values);
void render_upload_buffer(uint buffer, size_t size, void* data);

#endif