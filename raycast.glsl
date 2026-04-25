#version 430 core

#define INF 1e10
#define MAX_PORTAL_DEPTH 128
#define RAY_LENGTH 128

struct Portal {
    uint sector_index;
    float rotate, rotation_origin;
    vec2 translate;
};

struct Line {
    uint portal_start, portal_end;
    uint texture;
    vec2 texture_scale, texture_offset;
    vec2 a, b;
};

struct Sector {
    uint start, end;
    uint floor_texture, ceiling_texture;
    float floor_height, ceiling_height;
};

layout(std430, binding = 0) readonly buffer LineBuffer {
    Line lines[];
};

layout(std430, binding = 1) readonly buffer SectorBuffer {
    Sector sectors[];
};

layout(std430, binding = 2) readonly buffer PortalBuffer {
    Portal portals[];
};

const uint bayer_matrix[4][4] = {
    { 0, 8, 2, 10 },
    { 12, 4, 14, 6 },
    { 3, 11, 1, 9 },
    { 15, 7, 13, 5 },
};

uniform int curr_sector;
uniform sampler2D texture_atlas;
uniform vec3 player; // x, y -> pos, z -> rot
uniform float floor_offset;
uniform float fov = 3.14159 / 2;

uniform uint tileset_width = 16;
uniform uint tileset_height = 16;
uniform float fog_distance = 8;
uniform float fog_start = 0;
uniform vec3 fog_color = vec3(0);

uniform vec2 resolution = vec2(384, 256);
uniform float color_range = 3;

in vec2 uv;
out vec4 frag_color;

mat2 rotate(float a) {
    float s = sin(a);
    float c = cos(a);
    return mat2(c, s, -s, c);
}

float cross2d(vec2 a, vec2 b) {
    return a.x * b.y - a.y * b.x;
}

float map(float x, float src_f, float src_t, float dst_f, float dst_t) {
    return (x - src_f) / (src_t - src_f) * (dst_t - dst_f) + dst_f;
}

vec3 intersects_line(vec2 a1, vec2 a2, vec2 b1, vec2 b2) {
    vec2 r = a2 - a1;
    vec2 s = b2 - b1;
    
    vec2 ab = b1 - a1;
    float rxs = cross2d(r, s);
    float abxr = cross2d(ab, r);

    if (abs(rxs) < 1e-6) return vec3(0);

    float t1 = cross2d(ab, s) / rxs;
    float t2 = abxr / rxs;

    if (t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1)
        return vec3(a1 + t1 * r, 1.0);
    return vec3(0);
}

vec4 get_color(uint tile, vec2 pos, float dist) {
    pos = fract(pos);
    float fog = clamp((dist - fog_start) / fog_distance, 0, 1);
    uint tile_x = tile % tileset_width;
    uint tile_y = tile / tileset_height;
    vec2 tex = vec2(
        (tile_x + pos.x) / tileset_width,
        (tile_y + pos.y) / tileset_height
    );
    return vec4(mix(texture(texture_atlas, tex).rgb, fog_color, fog), 1);
}

vec4 get_floor_position(vec2 pos, float angle, float ver_pos, float floor_height, float added_dist, float player_angle) {
    if (ver_pos >= 0.5) return vec4(0);
    float height = -2 * ver_pos + 1;
    float dist = ((1 / height) * (1 - floor_height) - added_dist) / cos(player_angle - angle);
    return vec4(pos + vec2(1, 0) * rotate(angle) * dist, dist, 1);
}

vec4 get_ceiling_position(vec2 pos, float angle, float ver_pos, float height, float added_dist, float player_angle) {
    return get_floor_position(pos, angle, map(ver_pos, 0, 1, 1, 0), -height + 1, added_dist, player_angle);
}

void draw_floor_ceiling(inout vec4 color, uint sector, float floor_height, float ceiling_height, vec2 pos, float angle, float ver_pos, float added_dist, float player_angle) {
    vec4 world;
    uint texture;
    if (ver_pos >= 0.5) {
        world = get_ceiling_position(pos, angle, ver_pos, ceiling_height, added_dist, player_angle);
        texture = sectors[sector].ceiling_texture;
    }
    else {
        world = get_floor_position(pos, angle, ver_pos, floor_height, added_dist, player_angle);
        texture = sectors[sector].floor_texture;
    }
    if (world.w == 0) return;
    world.z += added_dist / cos(player_angle - angle);
    color = get_color(texture, fract(world.xy), world.z);
}

vec4 cast_ray(vec2 pos, float angle, float ver_pos) {
    float added_dist = 0;
    vec4 color = vec4(vec3(0), 1);
    uint sector = curr_sector;
    float player_angle = player.z;
    float floor_height = -floor_offset;
    for (uint i = 0; i < MAX_PORTAL_DEPTH; i++) {
        float sector_height = sectors[sector].ceiling_height - sectors[sector].floor_height + floor_height;
        float depth = INF, portal_dist;
        bool hit_portal = false;
        uint portal;
        vec3 portal_point;
        vec2 portal_new_pos = vec2(0);
        float portal_new_angle;
        float portal_new_player_angle;
        float portal_new_floor_height;
        for (uint l = sectors[sector].start; l < sectors[sector].end; l++) {
            vec3 point = intersects_line(pos, pos + vec2(RAY_LENGTH, 0) * rotate(angle), lines[l].a, lines[l].b);
            if (point.z == 0) continue;
            float dist = length(point.xy - pos) * cos(player_angle - angle);
            if (dist < 1e-4) continue;
            dist += added_dist;
            float height = 1 / dist;
            vec2 tex = vec2(
                length(point.xy - lines[l].a) / length(lines[l].b - lines[l].a),
                map(ver_pos, 0.5 + height * sector_height / 2, 0.5 - height * (-floor_height + 1) / 2, 0, 1)
            );
            if (depth <= dist || tex.y < 0 || tex.y > 1) continue;
            depth = dist;
            hit_portal = false;
            for (uint p = lines[l].portal_start; p < lines[l].portal_end; p++) {
                float floor_diff = sectors[portals[p].sector_index].floor_height - sectors[sector].floor_height;
                float ceiling_diff = sectors[portals[p].sector_index].ceiling_height - sectors[sector].ceiling_height;
                if (
                    ver_pos > 0.5 - height * (-floor_height + 1) / 2 + height * floor_diff / 2 &&
                    ver_pos < 0.5 + height * sector_height / 2 + height * ceiling_diff / 2
                ) {
                    hit_portal = true;
                    portal = p;
                    portal_point = point;
                    portal_dist = dist;
                    color = vec4(vec3(0), 1);

                    vec2 origin = mix(lines[l].a, lines[l].b, portals[p].rotation_origin);
                    vec2 local = portal_point.xy - origin + portals[p].translate;
                    vec2 transform = local * rotate(portals[p].rotate);
                    portal_new_pos = transform + origin;
                    portal_new_angle = angle + portals[p].rotate;
                    portal_new_player_angle = player_angle + portals[p].rotate;
                    portal_new_floor_height = floor_height + sectors[portals[p].sector_index].floor_height - sectors[sector].floor_height;
                    break;
                }
            }
            if (hit_portal) continue;
            color = get_color(lines[l].texture, tex * lines[l].texture_scale * vec2(1, sectors[sector].ceiling_height - sectors[sector].floor_height + 1) + lines[l].texture_offset, dist);
        }
        if (depth == INF) draw_floor_ceiling(color, sector, floor_height, sector_height, pos, angle, ver_pos, added_dist, player_angle);
        if (!hit_portal) break;

        added_dist = portal_dist;
        floor_height = portal_new_floor_height;
        sector = portals[portal].sector_index;
        pos = portal_new_pos;
        angle = portal_new_angle;
        player_angle = portal_new_player_angle;
    }
    return color;
}

vec4 dither(vec4 color, vec2 screen_pos) {
    screen_pos *= resolution;
    color *= color_range;
    uint x = uint(screen_pos.x) % 4;
    uint y = uint(screen_pos.y) % 4;
    vec3 i = floor(color).rgb;
    vec3 f = fract(color).rgb * 17;
    return vec4(
        i.r + (f.r <= bayer_matrix[y][x] ? 0 : 1),
        i.g + (f.g <= bayer_matrix[y][x] ? 0 : 1),
        i.b + (f.b <= bayer_matrix[y][x] ? 0 : 1),
        color.a
    ) / color_range;
}

void main() {
    vec2 screen = floor(uv * resolution) / resolution;

    float ray_angle = player.z + atan((screen.x - 0.5) * 2 * tan(fov / 2));
    vec4 color = cast_ray(player.xy, ray_angle, screen.y);
    
    frag_color = dither(color, uv);
}
