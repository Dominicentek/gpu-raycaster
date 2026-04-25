#ifndef VECTOR_H
#define VECTOR_H

typedef struct { _Alignas(8)
    float x;
    float y;
} vec2;

#define vec2(x, y) (vec2){ x, y }

vec2 vec_add(vec2 a, vec2 b);
vec2 vec_sub(vec2 a, vec2 b);
vec2 vec_mul(vec2 v, float x);
vec2 vec_div(vec2 v, float x);
float vec_dot(vec2 a, vec2 b);
float vec_cross(vec2 a, vec2 b);
float vec_len(vec2 v);
vec2 vec_lerp(vec2 a, vec2 b, float x);
vec2 vec_norm(vec2 v);

#endif