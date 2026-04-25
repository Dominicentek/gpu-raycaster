#version 430 core

layout(location = 0) in vec2 a_pos;

out vec2 uv;

void main() {
    gl_Position = vec4(a_pos, 0, 1);
    uv = (a_pos + vec2(1, 1)) * 0.5;
}
