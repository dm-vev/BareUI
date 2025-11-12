#include "ui_core.h"
#include "ui_hal_test.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    double x;
    double y;
    double z;
} vertex3_t;

static const vertex3_t cube_vertices[8] = {
    { -1.0, -1.0, -1.0 },
    {  1.0, -1.0, -1.0 },
    {  1.0,  1.0, -1.0 },
    { -1.0,  1.0, -1.0 },
    { -1.0, -1.0,  1.0 },
    {  1.0, -1.0,  1.0 },
    {  1.0,  1.0,  1.0 },
    { -1.0,  1.0,  1.0 },
};

static const int cube_faces[6][4] = {
    { 0, 1, 2, 3 },
    { 4, 5, 6, 7 },
    { 0, 1, 5, 4 },
    { 2, 3, 7, 6 },
    { 0, 3, 7, 4 },
    { 1, 2, 6, 5 },
};

static ui_color_t background_canvas[UI_FRAMEBUFFER_WIDTH * UI_FRAMEBUFFER_HEIGHT];
static ui_color_t badge_stamp[32 * 16];

static ui_color_t mix_color(ui_color_t base, double brightness)
{
    double clamped = fmax(0.0, fmin(1.0, brightness));
    const double ambient = 0.28;
    const double intensity = ambient + (1.0 - ambient) * clamped;

    double dr = (double)((base >> 11) & 0x1F) / 31.0;
    double dg = (double)((base >> 5) & 0x3F) / 63.0;
    double db = (double)(base & 0x1F) / 31.0;

    uint8_t r = (uint8_t)fmin(255.0, dr * intensity * 255.0);
    uint8_t g = (uint8_t)fmin(255.0, dg * intensity * 255.0);
    uint8_t b = (uint8_t)fmin(255.0, db * intensity * 255.0);
    return ui_color_rgb(r, g, b);
}

static vertex3_t normalized_light_dir(void)
{
    static vertex3_t dir = {0.0, 0.0, -1.0};
    static bool ready = false;
    if (!ready) {
        const vertex3_t raw = {0.5, -0.4, -0.75};
        double len = sqrt(raw.x * raw.x + raw.y * raw.y + raw.z * raw.z);
        if (len > 0.0) {
            dir.x = raw.x / len;
            dir.y = raw.y / len;
            dir.z = raw.z / len;
        }
        ready = true;
    }
    return dir;
}

static ui_color_t background_canvas[UI_FRAMEBUFFER_WIDTH * UI_FRAMEBUFFER_HEIGHT];

static void build_background(ui_color_t *canvas)
{
    for (int y = 0; y < UI_FRAMEBUFFER_HEIGHT; ++y) {
        double py = (double)y / (double)UI_FRAMEBUFFER_HEIGHT;
        for (int x = 0; x < UI_FRAMEBUFFER_WIDTH; ++x) {
            double px = (double)x / (double)UI_FRAMEBUFFER_WIDTH;
            double wave = 0.5 + 0.5 * sin(px * 6.283185307 + py * 3.141592653);
            double glow = 0.4 + 0.6 * wave;
            uint8_t r = (uint8_t)fmin(255.0, 32.0 + glow * 120.0);
            uint8_t g = (uint8_t)fmin(255.0, 48.0 + glow * 80.0);
            uint8_t b = (uint8_t)fmin(255.0, 72.0 + glow * 60.0);
            canvas[y * UI_FRAMEBUFFER_WIDTH + x] = ui_color_rgb(r, g, b);
        }
    }
}

static void build_badge(ui_color_t *stamp, int width, int height)
{
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double t = (double)x / (double)width;
            double blend = 0.3 + 0.7 * t;
            uint8_t r = (uint8_t)fmin(255.0, 64.0 + blend * 160.0);
            uint8_t g = (uint8_t)fmin(255.0, 96.0 + blend * 120.0);
            uint8_t b = (uint8_t)fmin(255.0, 120.0 + blend * 80.0);
            stamp[y * width + x] = ui_color_rgb(r, g, b);
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (((x / 4) + (y / 4)) % 2 == 0) {
                stamp[y * width + x] = ui_color_from_hex(0x102942);
            }
        }
    }
}

static void draw_demo_polygon(ui_context_t *ctx, double phase)
{
    ui_point_t polygon[6];
    const double radius = 48.0;
    const double center_x = 76.0;
    const double center_y = 90.0;
    for (int i = 0; i < 6; ++i) {
        double angle = phase + (double)i * 1.0471975512; /* 60 degrees */
        polygon[i].x = (int16_t)(center_x + radius * cos(angle));
        polygon[i].y = (int16_t)(center_y + radius * sin(angle));
    }
    ui_context_draw_polygon(ctx, polygon, 6, ui_color_from_hex(0xF6D55C));
}

static void draw_pixel_grid(ui_context_t *ctx)
{
    const int offset_x = UI_FRAMEBUFFER_WIDTH - 64;
    const int offset_y = 24;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            ui_color_t color = ((x ^ y) & 1) ? ui_color_from_hex(0xFF6B6B) : ui_color_from_hex(0x00A8A8);
            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < 2; ++dx) {
                    ui_context_set_pixel(ctx, offset_x + x * 2 + dx, offset_y + y * 2 + dy, color);
                }
            }
        }
    }
}

static void draw_cube(ui_context_t *ctx, double angle)
{
    vertex3_t rotated[8];
    double projections_z[8];
    ui_point_t projected[8];

    double cos_a = cos(angle);
    double sin_a = sin(angle);
    double cos_b = cos(angle * 0.6);
    double sin_b = sin(angle * 0.6);
    const double offset = 4.0;
    const double scale = 120.0;

    for (int i = 0; i < 8; ++i) {
        double x = cube_vertices[i].x;
        double y = cube_vertices[i].y;
        double z = cube_vertices[i].z;

        double x1 = x * cos_a - z * sin_a;
        double z1 = x * sin_a + z * cos_a;
        double y1 = y;

        double y2 = y1 * cos_b - z1 * sin_b;
        double z2 = y1 * sin_b + z1 * cos_b;

        rotated[i].x = x1;
        rotated[i].y = y2;
        rotated[i].z = z2;

        double world_z = z2 + offset;
        projections_z[i] = world_z;
        double inv = 1.0 / world_z;
        double px = UI_FRAMEBUFFER_WIDTH / 2.0 + x1 * inv * scale;
        double py = UI_FRAMEBUFFER_HEIGHT / 2.0 - y2 * inv * scale;
        projected[i].x = (int16_t)round(px);
        projected[i].y = (int16_t)round(py);
    }

    typedef struct {
        double depth;
        ui_color_t color;
        ui_point_t pts[4];
    } face_draw_t;

    face_draw_t draw_list[6];
    size_t draw_count = 0;

    for (size_t face = 0; face < 6; ++face) {
        int idx0 = cube_faces[face][0];
        int idx1 = cube_faces[face][1];
        int idx2 = cube_faces[face][2];

        const vertex3_t *v0 = &rotated[idx0];
        const vertex3_t *v1 = &rotated[idx1];
        const vertex3_t *v2 = &rotated[idx2];

        vertex3_t edge1 = {
            v1->x - v0->x,
            v1->y - v0->y,
            v1->z - v0->z,
        };
        vertex3_t edge2 = {
            v2->x - v0->x,
            v2->y - v0->y,
            v2->z - v0->z,
        };
        vertex3_t normal = {
            edge1.y * edge2.z - edge1.z * edge2.y,
            edge1.z * edge2.x - edge1.x * edge2.z,
            edge1.x * edge2.y - edge1.y * edge2.x,
        };
        double norm_len = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        if (norm_len == 0.0) {
            continue;
        }
        normal.x /= norm_len;
        normal.y /= norm_len;
        normal.z /= norm_len;
        vertex3_t light = normalized_light_dir();
        double lighting = normal.x * light.x + normal.y * light.y + normal.z * light.z;
        const double ambient = 0.22;
        double diff = fmax(0.0, fmin(1.0, lighting));
        double brightness = ambient + (1.0 - ambient) * diff;
        ui_color_t face_color = mix_color(ui_color_from_hex(0x5FA9FF), brightness);

        face_draw_t *entry = &draw_list[draw_count++];
        entry->depth = (projections_z[idx0] + projections_z[idx1] +
                        projections_z[idx2] + projections_z[cube_faces[face][3]]) * 0.25;
        entry->color = face_color;
        for (size_t k = 0; k < 4; ++k) {
            entry->pts[k] = projected[cube_faces[face][k]];
        }
    }

    for (size_t i = 0; i < draw_count; ++i) {
        for (size_t j = i + 1; j < draw_count; ++j) {
            if (draw_list[j].depth < draw_list[i].depth) {
                face_draw_t tmp = draw_list[i];
                draw_list[i] = draw_list[j];
                draw_list[j] = tmp;
            }
        }
    }

    for (size_t i = 0; i < draw_count; ++i) {
        ui_context_draw_polygon(ctx, draw_list[i].pts, 4, draw_list[i].color);
    }
}

int main(void)
{
    const ui_hal_ops_t *hal = ui_hal_test_sdl_ops();
    ui_context_t *ctx = ui_context_create(hal);
    if (!ctx) {
        return 1;
    }

    bool running = true;

    build_background(background_canvas);
    build_badge(badge_stamp, 32, 16);

    while (running) {
        ui_event_t event;
        while (ui_context_poll_event(ctx, &event)) {
            if (event.type == UI_EVENT_QUIT) {
                running = false;
            }
            if (event.type == UI_EVENT_KEY_DOWN) {
                if (event.data.key.keycode == 'q' || event.data.key.keycode == 'Q') {
                    running = false;
                }
            }
        }

        ui_context_blit(ctx, background_canvas, UI_FRAMEBUFFER_WIDTH,
                        UI_FRAMEBUFFER_HEIGHT, 0, 0);
        double seconds = (double)clock() / CLOCKS_PER_SEC;
        draw_cube(ctx, seconds * 0.8);
        draw_demo_polygon(ctx, seconds * 0.7);
        draw_pixel_grid(ctx);
        ui_context_draw_text(ctx, 12, UI_FRAMEBUFFER_HEIGHT - 24,
                             "BareUI demo: fonts + blit + polygon + cube",
                             ui_color_from_hex(0xE8E8E8));
        ui_context_draw_text(ctx, 12, 12, "ASCII + кириллица: Пример текста",
                             ui_color_from_hex(0xFFE66D));
        ui_context_draw_text(ctx, 12, 26, "Пиксели и полигоны следуют:",
                             ui_color_from_hex(0xC7F9CC));
        ui_context_blit(ctx, badge_stamp, 32, 16, UI_FRAMEBUFFER_WIDTH - 40,
                        UI_FRAMEBUFFER_HEIGHT - 24);
        ui_context_render(ctx);

        struct timespec wait = {0, 16 * 1000000};
        nanosleep(&wait, NULL);
    }

    ui_context_destroy(ctx);
    return 0;
}
