#include "vector.h"

#include <math.h>

vec2 vec_add(vec2 a, vec2 b) {
    return (vec2){ a.x + b.x, a.y + b.y };
}

vec2 vec_sub(vec2 a, vec2 b) {
    return (vec2){ a.x - b.x, a.y - b.y };
}

vec2 vec_mul(vec2 v, float x) {
    return (vec2){ v.x * x, v.y * x };
}

vec2 vec_div(vec2 v, float x) {
    return (vec2){ v.x / x, v.y / x };
}

float vec_dot(vec2 a, vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float vec_cross(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

float vec_len(vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

vec2 vec_lerp(vec2 a, vec2 b, float x) {
    return (vec2){
        (b.x - a.x) * x + a.x,
        (b.y - a.y) * x + a.y,
    };
}

vec2 vec_norm(vec2 v) {
    if (v.x == 0 && v.y == 0) return v;
    return vec_div(v, vec_len(v));
}
