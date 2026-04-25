#include "render.h"
#include "vector.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEG2RAD(x) ((x) / 360.f * 2 * 3.14159f)

static struct {
    union {
        vec2 xy;
        struct {
            float x, y;
        };
    };
    float angle;
} player;

static int line_buffer;
static int sector_buffer;
static int portal_buffer;
static int curr_sector;
static float floor_offset;
static float fall_speed;

static float fov = DEG2RAD(90);

typedef struct {
    uint sector_index;
    float rotate, rotation_origin;
    vec2 translate;
} Portal;

typedef struct {
    uint portal_start, portal_end;
    uint texture;
    vec2 texture_scale, texture_offset;
    vec2 a, b;
} Line;

typedef struct {
    uint start, end;
    uint floor_texture, ceiling_texture;
    float floor_height, ceiling_height;
} Sector;

static Line* lines;
static Sector* sectors;
static Portal* portals;

void load_scene(const char* filename) {
    FILE* f = fopen(filename, "r");
    uint num_lines, num_sectors, num_portals;
    float fog_start = 0, fog_dist = 8, fog_color[3] = { 0, 0, 0 };
    fscanf(f, "%u %u %u %u %f %f %f %f %f\n", &num_lines, &num_sectors, &num_portals, &curr_sector, &fog_start, &fog_dist, &fog_color[0], &fog_color[1], &fog_color[2]);
    
    free(lines);
    free(sectors);
    free(portals);

    Line* line = lines = calloc(sizeof(Line), num_lines);
    Sector* sector = sectors = calloc(sizeof(Sector), num_sectors);
    Portal* portal = portals = calloc(sizeof(Portal), num_portals);
    for (uint i = 0; i < num_sectors; i++)
        sectors[i].ceiling_height = 1;
    for (uint i = 0; i < num_lines; i++)
        lines[i].texture_scale = vec2(1, 1);
    for (uint i = 0; i < num_portals; i++)
        portals[i].rotation_origin = 0.5f;
    
    char item; int scanned;
    while ((item = 0, scanned = fscanf(f, "%c ", &item)) != EOF) {
        if (scanned != 0) switch (item) {
            case 'l': if (line - lines < num_lines) fscanf(f, "%f %f %f %f %u %f %f %f %f %u %u\n",
                &line->a.x, &line->a.y, &line->b.x, &line->b.y,
                &line->texture, &line->texture_scale.x, &line->texture_scale.y, &line->texture_offset.x, &line->texture_offset.y,
                &line->portal_start, &line->portal_end
            ), line++; break;
            case 's': if (sector - sectors < num_sectors) fscanf(f, "%u %u %f %f %u %u\n",
                &sector->start, &sector->end,
                &sector->floor_height, &sector->ceiling_height,
                &sector->floor_texture, &sector->ceiling_texture
            ), sector++; break;
            case 'p': if (portal - portals < num_portals) fscanf(f, "%u %f %f %f %f\n",
                &portal->sector_index, &portal->translate.x, &portal->translate.y,
                &portal->rotate, &portal->rotation_origin
            ), portal++; break;
        }
        else fscanf(f, "%*[^\n]\n");
    }
    
    render_upload_buffer(line_buffer, sizeof(Line) * num_lines, lines);
    render_upload_buffer(sector_buffer, sizeof(Sector) * num_sectors, sectors);
    render_upload_buffer(portal_buffer, sizeof(Portal) * num_portals, portals);
    render_upload_float("fog_start", fog_start);
    render_upload_float("fog_distance", fog_dist);
    render_upload_float3("fog_color", fog_color);

    fclose(f);
}

bool intersects_line(vec2 a1, vec2 a2, vec2 b1, vec2 b2, vec2* out) {
    vec2 r = vec_sub(a2, a1);
    vec2 s = vec_sub(b2, b1);
    
    vec2 ab = vec_sub(b1, a1);
    float rxs = vec_cross(r, s);
    float abxr = vec_cross(ab, r);

    if (fabsf(rxs) < 1e-6) return false;

    float t1 = vec_cross(ab, s) / rxs;
    float t2 = abxr / rxs;

    if (t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1) {
        if (out) *out = vec_add(a1, vec_mul(r, t1));
        return true;
    }
    return false;
}

float distance_to_line(vec2 p, vec2 a, vec2 b, vec2* closest) {
    vec2 _;
    if (!closest) closest = &_;
    
    vec2 r = vec_sub(b, a);
    vec2 s = vec_sub(p, a);
    float dot_sr = vec_dot(s, r);
    float dot_rr = vec_dot(r, r);
    float t = dot_rr == 0 || dot_sr == 0 ? 0 : dot_sr / dot_rr;

    if (t < 0) t = 0;
    if (t > 1) t = 1;
    *closest = vec_lerp(a, b, t);
    return vec_len(vec_sub(p, *closest));
}

Portal* get_portal(Line* line) {
    for (uint i = line->portal_start; i < line->portal_end; i++) {
        Portal* portal = &portals[i];
        float new_floor_height = sectors[portal->sector_index].floor_height - sectors[curr_sector].floor_height - floor_offset;
        if (
            sectors[portal->sector_index].floor_height - sectors[curr_sector].floor_height - floor_offset < 0.5 &&
            sectors[portal->sector_index].ceiling_height - sectors[portal->sector_index].floor_height + new_floor_height > 0.5
        ) return portal;
    }
    return NULL;
}

#define COLLBOX_SIZE 0.3
void update_physics(double dt) {
    static vec2 prev_pos;
    float ceil_height = sectors[curr_sector].ceiling_height - sectors[curr_sector].floor_height - floor_offset;
    for (int i = sectors[curr_sector].start; i < sectors[curr_sector].end; i++) {
        Portal* portal = get_portal(&lines[i]);
        if (portal) {
            if (!intersects_line(prev_pos, player.xy, lines[i].a, lines[i].b, NULL)) continue;
            floor_offset += sectors[curr_sector].floor_height - sectors[portal->sector_index].floor_height;
            curr_sector = portal->sector_index;
            break;
        }
        else {
            float dist = distance_to_line(player.xy, lines[i].a, lines[i].b, NULL);
            if (dist > COLLBOX_SIZE) continue;
            
            vec2 half = vec_lerp(lines[i].a, lines[i].b, 0.5f);
            vec2 line = vec_sub(lines[i].b, lines[i].a);
            vec2 normal = vec_norm(vec2(-line.y, line.x));
            if (vec_dot(normal, vec_norm(vec_sub(player.xy, half))) < 0) normal = vec_mul(normal, -1);
    
            float penetration = COLLBOX_SIZE - dist;
            player.xy = vec_add(player.xy, vec_mul(normal, penetration));
        }
    }
    fall_speed += 9.8 * dt;
    floor_offset -= fall_speed * dt;
    if (ceil_height < 0.5 && fall_speed < 0) fall_speed = 0;
    if (floor_offset < 0) {
        floor_offset = 0;
        fall_speed = 0;
    }
    prev_pos = player.xy;
}

void update(double dt) {
    static double prev_mouse_x, prev_mouse_y;
    static bool first_frame = true;
    double mouse_x, mouse_y;
    float x = 0, y = 0;
    if (glfwGetKey(window, GLFW_KEY_W)) x += dt * 3;
    if (glfwGetKey(window, GLFW_KEY_S)) x -= dt * 3;
    if (glfwGetKey(window, GLFW_KEY_A)) y += dt * 3;
    if (glfwGetKey(window, GLFW_KEY_D)) y -= dt * 3;
    if (glfwGetKey(window, GLFW_KEY_SPACE) && floor_offset == 0) fall_speed = -5;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    if (!first_frame) {
        double delta = mouse_x - prev_mouse_x;
        player.angle += delta * 0.01;
        while (player.angle <  0)            player.angle += 2 * 3.14159f;
        while (player.angle >= 2 * 3.14159f) player.angle -= 2 * 3.14159f;
        prev_mouse_x = mouse_x;
        prev_mouse_y = mouse_y;
    }
    float rot_x = x * cosf(-player.angle) - y * sinf(-player.angle);
    float rot_y = x * sinf(-player.angle) + y * cosf(-player.angle);
    first_frame = false;
    player.xy = vec_add(player.xy, vec2(rot_x, rot_y));
    update_physics(dt);
    render_upload_float3("player", &player);
    render_upload_int("curr_sector", curr_sector);
    render_upload_float("floor_offset", floor_offset);
    
    int win_x, win_y;
    glfwGetWindowSize(window, &win_x, &win_y);
    float ratio = (float)win_x / (float)win_y;
    render_upload_float("fov", 2 * atanf(tanf(fov / 2) * ratio));
    
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE)) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

int main(int argc, char** argv) {
    const char* scene = "scene.txt";
    if (argc > 1) scene = argv[1];

    render_init("raycast.glsl", "vertex.glsl");
    line_buffer = render_bind_buffer(0);
    sector_buffer = render_bind_buffer(1);
    portal_buffer = render_bind_buffer(2);
    render_use_texture(render_bind_texture("textures.png"));
    load_scene("scene.txt");
    while (!render_quit()) render_update(update);
    return 0;
}