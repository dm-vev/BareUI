#include "ui_core.h"
#include "ui_hal_test.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

#define PIXEL_GRID_WIDTH 16
#define PIXEL_GRID_HEIGHT 16
#define OVERLAY_TRANSPARENT ((ui_color_t)0xFFFF)
#define TEXT_SPRITE_STR_MAX 128
#define TEXT_SPRITE_WIDTH UI_FRAMEBUFFER_WIDTH
#define TEXT_SPRITE_HEIGHT (BAREUI_FONT_HEIGHT)

static ui_color_t pixel_grid_stamp[PIXEL_GRID_WIDTH * PIXEL_GRID_HEIGHT];
static ui_color_t status_text_pixels[TEXT_SPRITE_WIDTH * TEXT_SPRITE_HEIGHT];
static ui_color_t instruction_text_pixels[TEXT_SPRITE_WIDTH * TEXT_SPRITE_HEIGHT];
static ui_color_t footer_text_pixels[TEXT_SPRITE_WIDTH * TEXT_SPRITE_HEIGHT];

typedef struct {
    ui_color_t *pixels;
    int stride;
    int width;
    int height;
    int x;
    int y;
    ui_color_t color;
    int drawn_width;
    char last_text[TEXT_SPRITE_STR_MAX];
} text_sprite_t;

static text_sprite_t status_sprite;
static text_sprite_t instruction_sprite;
static text_sprite_t footer_sprite;

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

static void build_pixel_grid_stamp(ui_color_t *stamp, int width, int height)
{
    if (!stamp || width <= 0 || height <= 0) {
        return;
    }
    const int cell_size = 2;
    for (int y_cell = 0; y_cell < 8; ++y_cell) {
        for (int x_cell = 0; x_cell < 8; ++x_cell) {
            ui_color_t color = ((x_cell ^ y_cell) & 1) ? ui_color_from_hex(0xFF6B6B)
                                                        : ui_color_from_hex(0x00A8A8);
            for (int dy = 0; dy < cell_size; ++dy) {
                for (int dx = 0; dx < cell_size; ++dx) {
                    int px = x_cell * cell_size + dx;
                    int py = y_cell * cell_size + dy;
                    if (px >= width || py >= height) {
                        continue;
                    }
                    stamp[py * width + px] = color;
                }
            }
        }
    }
}

static void text_sprite_init(text_sprite_t *sprite, ui_color_t *buffer, int width,
                             int height, int x, int y, ui_color_t color)
{
    if (!sprite || !buffer || width <= 0 || height <= 0) {
        return;
    }
    sprite->pixels = buffer;
    sprite->stride = width;
    sprite->width = width;
    sprite->height = height;
    sprite->x = x;
    sprite->y = y;
    sprite->color = color;
    sprite->drawn_width = 0;
    sprite->last_text[0] = '\0';
    size_t area = (size_t)width * height;
    for (size_t i = 0; i < area; ++i) {
        buffer[i] = OVERLAY_TRANSPARENT;
    }
}

static bool overlay_next_codepoint(const char **text, uint32_t *out)
{
    if (!text || !*text || !out) {
        return false;
    }
    const unsigned char *ptr = (const unsigned char *)*text;
    uint32_t cp = *ptr++;
    if (cp < 0x80) {
        *out = cp;
        *text = (const char *)ptr;
        return true;
    }

    int extra = 0;
    if ((cp & 0xE0) == 0xC0) {
        cp &= 0x1F;
        extra = 1;
    } else if ((cp & 0xF0) == 0xE0) {
        cp &= 0x0F;
        extra = 2;
    } else if ((cp & 0xF8) == 0xF0) {
        cp &= 0x07;
        extra = 3;
    } else {
        return false;
    }

    for (int i = 0; i < extra; ++i) {
        if ((ptr[i] & 0xC0) != 0x80) {
            return false;
        }
        cp = (cp << 6) | (ptr[i] & 0x3F);
    }

    *text = (const char *)(ptr + extra);
    *out = cp;
    return true;
}

static void text_sprite_update(text_sprite_t *sprite, const char *text)
{
    if (!sprite || !sprite->pixels || !text) {
        return;
    }
    if (strncmp(sprite->last_text, text, sizeof(sprite->last_text)) == 0) {
        return;
    }
    size_t area = (size_t)sprite->stride * sprite->height;
    for (size_t i = 0; i < area; ++i) {
        sprite->pixels[i] = OVERLAY_TRANSPARENT;
    }
    const bareui_font_t *font = bareui_font_default();
    int cursor = 0;
    const char *ptr = text;
    uint32_t codepoint = 0;
    while (cursor < sprite->width && overlay_next_codepoint(&ptr, &codepoint)) {
        if (codepoint == '\n') {
            cursor = 0;
            continue;
        }
        bareui_font_glyph_t glyph;
        if (!bareui_font_lookup(font, codepoint, &glyph)) {
            continue;
        }
        for (uint8_t col = 0; col < glyph.width; ++col) {
            uint8_t column = glyph.columns[col];
            for (uint8_t row = 0; row < glyph.height; ++row) {
                if (!(column & (1u << row))) {
                    continue;
                }
                int px = cursor + col;
                int py = row;
                if (px < 0 || px >= sprite->width || py < 0 || py >= sprite->height) {
                    continue;
                }
                sprite->pixels[py * sprite->stride + px] = sprite->color;
            }
        }
        cursor += glyph.spacing;
    }
    sprite->drawn_width = cursor < sprite->width ? cursor : sprite->width;
    strncpy(sprite->last_text, text, sizeof(sprite->last_text) - 1);
    sprite->last_text[sizeof(sprite->last_text) - 1] = '\0';
}

static void text_sprite_draw(const text_sprite_t *sprite, ui_context_t *ctx)
{
    if (!sprite || !sprite->pixels || !ctx) {
        return;
    }
    for (int row = 0; row < sprite->height; ++row) {
        const ui_color_t *row_pixels = sprite->pixels + (size_t)row * sprite->stride;
        for (int col = 0; col < sprite->drawn_width; ++col) {
            ui_color_t pixel = row_pixels[col];
            if (pixel == OVERLAY_TRANSPARENT) {
                continue;
            }
            ui_context_set_pixel(ctx, sprite->x + col, sprite->y + row, pixel);
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
        double angle = phase + (double)i * 1.0471975512;
        polygon[i].x = (int16_t)(center_x + radius * cos(angle));
        polygon[i].y = (int16_t)(center_y + radius * sin(angle));
    }
    ui_context_draw_polygon(ctx, polygon, 6, ui_color_from_hex(0xF6D55C));
}

static void draw_pixel_grid(ui_context_t *ctx)
{
    ui_context_blit(ctx, pixel_grid_stamp, PIXEL_GRID_WIDTH, PIXEL_GRID_HEIGHT,
                    UI_FRAMEBUFFER_WIDTH - 64, 24);
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

    build_background(background_canvas);
    build_badge(badge_stamp, 32, 16);
    build_pixel_grid_stamp(pixel_grid_stamp, PIXEL_GRID_WIDTH, PIXEL_GRID_HEIGHT);
    text_sprite_init(&status_sprite, status_text_pixels, TEXT_SPRITE_WIDTH, TEXT_SPRITE_HEIGHT,
                     12, 12, ui_color_from_hex(0xFFE66D));
    text_sprite_init(&instruction_sprite, instruction_text_pixels, TEXT_SPRITE_WIDTH,
                     TEXT_SPRITE_HEIGHT, 12, 24, ui_color_from_hex(0xC7F9CC));
    text_sprite_init(&footer_sprite, footer_text_pixels, TEXT_SPRITE_WIDTH, TEXT_SPRITE_HEIGHT,
                     12, UI_FRAMEBUFFER_HEIGHT - 24, ui_color_from_hex(0xE8E8E8));
    text_sprite_update(&instruction_sprite, "B=regenerate background · R=badge · Q=quit");
    text_sprite_update(&footer_sprite, "BareUI demo: fonts · blit · polygon · cube");
    ui_context_blit(ctx, background_canvas, UI_FRAMEBUFFER_WIDTH,
                    UI_FRAMEBUFFER_HEIGHT, 0, 0);

    bool running = true;
    bool scroll_left = false;
    bool scroll_right = false;
    bool scroll_up = false;
    bool scroll_down = false;

    while (running) {
        ui_event_t event;
        while (ui_context_poll_event(ctx, &event)) {
            if (event.type == UI_EVENT_QUIT) {
                running = false;
            } else if (event.type == UI_EVENT_KEY_DOWN) {
                switch (event.data.key.keycode) {
                case 'a':
                case 'A':
                    scroll_left = true;
                    break;
                case 'd':
                case 'D':
                    scroll_right = true;
                    break;
                case 'w':
                case 'W':
                    scroll_up = true;
                    break;
                case 's':
                case 'S':
                    scroll_down = true;
                    break;
                case 'b':
                case 'B':
                    build_background(background_canvas);
                    ui_context_blit(ctx, background_canvas, UI_FRAMEBUFFER_WIDTH,
                                    UI_FRAMEBUFFER_HEIGHT, 0, 0);
                    break;
                case 'r':
                case 'R':
                    build_badge(badge_stamp, 32, 16);
                    break;
                case 'q':
                case 'Q':
                    running = false;
                    break;
                default:
                    break;
                }
            } else if (event.type == UI_EVENT_KEY_UP) {
                switch (event.data.key.keycode) {
                case 'a':
                case 'A':
                    scroll_left = false;
                    break;
                case 'd':
                case 'D':
                    scroll_right = false;
                    break;
                case 'w':
                case 'W':
                    scroll_up = false;
                    break;
                case 's':
                case 'S':
                    scroll_down = false;
                    break;
                default:
                    break;
                }
            }
        }

        int scroll_dx = (int)scroll_right - (int)scroll_left;
        int scroll_dy = (int)scroll_down - (int)scroll_up;
        if (scroll_dx || scroll_dy) {
            ui_context_scroll(ctx, scroll_dx, scroll_dy, ui_color_from_hex(0x040B16));
        }

        double seconds = (double)clock() / CLOCKS_PER_SEC;
        draw_cube(ctx, seconds * 0.8);
        draw_demo_polygon(ctx, seconds * 0.7);
        draw_pixel_grid(ctx);

        char status[64];
        snprintf(status, sizeof(status), "Scroll vector %d, %d (arrows move cache)",
                 scroll_dx, scroll_dy);
        text_sprite_update(&status_sprite, status);
        text_sprite_draw(&status_sprite, ctx);
        text_sprite_draw(&instruction_sprite, ctx);
        text_sprite_draw(&footer_sprite, ctx);
        ui_context_blit(ctx, badge_stamp, 32, 16, UI_FRAMEBUFFER_WIDTH - 40,
                        UI_FRAMEBUFFER_HEIGHT - 24);

        ui_context_render(ctx);

        struct timespec wait = {0, 16 * 1000000};
        nanosleep(&wait, NULL);
    }

    ui_context_destroy(ctx);
    return 0;
}
