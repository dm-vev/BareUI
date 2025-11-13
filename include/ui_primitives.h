#ifndef UI_PRIMITIVES_H
#define UI_PRIMITIVES_H

#include "ui_font.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UI_FRAMEBUFFER_WIDTH 320
#define UI_FRAMEBUFFER_HEIGHT 240

typedef uint16_t ui_color_t;

static inline ui_color_t ui_color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((((uint16_t)(r) & 0xF8) << 8) | (((uint16_t)(g) & 0xFC) << 3) |
                      (((uint16_t)(b) & 0xF8) >> 3));
}

static inline ui_color_t ui_color_from_hex(uint32_t hex)
{
    return ui_color_rgb((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}

typedef enum {
    UI_EVENT_TOUCH_DOWN,
    UI_EVENT_TOUCH_UP,
    UI_EVENT_TOUCH_MOVE,
    UI_EVENT_KEY_DOWN,
    UI_EVENT_KEY_UP,
    UI_EVENT_QUIT
} ui_event_type_t;

typedef struct {
    ui_event_type_t type;
    union {
        struct {
            int16_t x;
            int16_t y;
        } touch;
        struct {
            uint32_t keycode;
        } key;
    } data;
} ui_event_t;

typedef struct {
    int16_t x;
    int16_t y;
} ui_point_t;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} ui_rect_t;

typedef struct ui_context ui_context_t;

typedef struct {
    void *user_data;
    bool (*init)(ui_context_t *ctx);
    void (*deinit)(ui_context_t *ctx);
    void (*commit_frame)(ui_context_t *ctx, const ui_color_t *framebuffer);
} ui_hal_ops_t;

ui_context_t *ui_context_create(const ui_hal_ops_t *hal);
void ui_context_destroy(ui_context_t *ctx);

void ui_context_clear(ui_context_t *ctx, ui_color_t color);
void ui_context_fill_rect(ui_context_t *ctx, int x, int y, int width, int height,
                          ui_color_t color);
void ui_context_set_pixel(ui_context_t *ctx, int x, int y, ui_color_t color);
void ui_context_draw_codepoint(ui_context_t *ctx, int x, int y, uint32_t codepoint,
                               ui_color_t color);
void ui_context_draw_text(ui_context_t *ctx, int x, int y, const char *text,
                          ui_color_t color);
void ui_context_draw_polygon(ui_context_t *ctx, const ui_point_t *points,
                             size_t point_count, ui_color_t color);
bool ui_context_blit(ui_context_t *ctx, const ui_color_t *src, int src_width,
                     int src_height, int dst_x, int dst_y);
bool ui_context_scroll(ui_context_t *ctx, int dx, int dy, ui_color_t fill);

bool ui_context_poll_event(ui_context_t *ctx, ui_event_t *event);
bool ui_context_post_event(ui_context_t *ctx, const ui_event_t *event);

void ui_context_render(ui_context_t *ctx);

void *ui_context_user_data(ui_context_t *ctx);
void ui_context_set_user_data(ui_context_t *ctx, void *data);

const ui_hal_ops_t *ui_context_hal(const ui_context_t *ctx);

const bareui_font_t *ui_context_font(const ui_context_t *ctx);
void ui_context_set_font(ui_context_t *ctx, const bareui_font_t *font);

#endif
