#include "ui_core.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
/* ASCII 5x7 fast path disabled for now; rely on font lookup */

#define UI_EVENT_QUEUE_SIZE 128

struct ui_context {
    ui_color_t framebuffer[UI_FRAMEBUFFER_WIDTH * UI_FRAMEBUFFER_HEIGHT];
    pthread_mutex_t fb_lock;
    pthread_mutex_t ev_lock;
    ui_event_t ev_queue[UI_EVENT_QUEUE_SIZE];
    size_t ev_head;
    size_t ev_count;
    const ui_hal_ops_t *hal;
    void *user_data;
    const bareui_font_t *font;
    bool dirty;
    int dirty_min_x;
    int dirty_min_y;
    int dirty_max_x;
    int dirty_max_y;
};

static inline void ui_set_pixel_locked(ui_context_t *ctx, int x, int y, ui_color_t color)
{
    if ((unsigned)x >= UI_FRAMEBUFFER_WIDTH || (unsigned)y >= UI_FRAMEBUFFER_HEIGHT) {
        return;
    }
    ctx->framebuffer[y * UI_FRAMEBUFFER_WIDTH + x] = color;
}

static void ui_reset_dirty(ui_context_t *ctx)
{
    ctx->dirty = false;
    ctx->dirty_min_x = UI_FRAMEBUFFER_WIDTH;
    ctx->dirty_min_y = UI_FRAMEBUFFER_HEIGHT;
    ctx->dirty_max_x = 0;
    ctx->dirty_max_y = 0;
}

static void ui_mark_dirty_locked(ui_context_t *ctx, int x, int y, int width, int height)
{
    if (!ctx || width <= 0 || height <= 0) {
        return;
    }
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + width;
    int y1 = y + height;
    if (x1 > UI_FRAMEBUFFER_WIDTH) {
        x1 = UI_FRAMEBUFFER_WIDTH;
    }
    if (y1 > UI_FRAMEBUFFER_HEIGHT) {
        y1 = UI_FRAMEBUFFER_HEIGHT;
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }
    if (!ctx->dirty) {
        ctx->dirty = true;
        ctx->dirty_min_x = x0;
        ctx->dirty_min_y = y0;
        ctx->dirty_max_x = x1;
        ctx->dirty_max_y = y1;
        return;
    }
    if (x0 < ctx->dirty_min_x) {
        ctx->dirty_min_x = x0;
    }
    if (y0 < ctx->dirty_min_y) {
        ctx->dirty_min_y = y0;
    }
    if (x1 > ctx->dirty_max_x) {
        ctx->dirty_max_x = x1;
    }
    if (y1 > ctx->dirty_max_y) {
        ctx->dirty_max_y = y1;
    }
}

static bool ui_next_codepoint(const char **text, uint32_t *out)
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

static bool ui_context_get_glyph(ui_context_t *ctx, uint32_t codepoint,
                                 bareui_font_glyph_t *glyph)
{
    if (!ctx || !glyph || !ctx->font) {
        return false;
    }
    if (bareui_font_lookup(ctx->font, codepoint, glyph)) {
        return true;
    }
    if (codepoint != '?') {
        return bareui_font_lookup(ctx->font, '?', glyph);
    }
    return false;
}

static void ui_draw_glyph_locked(ui_context_t *ctx, int x, int y,
                                 const bareui_font_glyph_t *glyph, ui_color_t color)
{
    if (!ctx || !glyph || !glyph->columns) {
        return;
    }

    for (uint8_t col = 0; col < glyph->width; ++col) {
        uint8_t column = glyph->columns[col];
        for (uint8_t row = 0; row < glyph->height; ++row) {
            if (column & (1u << row)) {
                ui_set_pixel_locked(ctx, x + col, y + row, color);
            }
        }
    }
    ui_mark_dirty_locked(ctx, x, y, glyph->width, glyph->height);
}

/* removed */

ui_context_t *ui_context_create(const ui_hal_ops_t *hal)
{
    if (!hal || !hal->init || !hal->commit_frame) {
        return NULL;
    }

    ui_context_t *ctx = malloc(sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }

    memset(ctx->framebuffer, 0, sizeof(ctx->framebuffer));
    pthread_mutex_init(&ctx->fb_lock, NULL);
    pthread_mutex_init(&ctx->ev_lock, NULL);
    ctx->ev_head = 0;
    ctx->ev_count = 0;
    ctx->hal = hal;
    ctx->user_data = hal->user_data;
    ctx->font = bareui_font_default();

    if (!hal->init(ctx)) {
        pthread_mutex_destroy(&ctx->ev_lock);
        pthread_mutex_destroy(&ctx->fb_lock);
        free(ctx);
        return NULL;
    }

    return ctx;
}

void ui_context_destroy(ui_context_t *ctx)
{
    if (!ctx) {
        return;
    }
    if (ctx->hal && ctx->hal->deinit) {
        ctx->hal->deinit(ctx);
    }
    pthread_mutex_destroy(&ctx->ev_lock);
    pthread_mutex_destroy(&ctx->fb_lock);
    free(ctx);
}

void ui_context_clear(ui_context_t *ctx, ui_color_t color)
{
    if (!ctx) {
        return;
    }
    pthread_mutex_lock(&ctx->fb_lock);
    for (size_t i = 0; i < UI_FRAMEBUFFER_WIDTH * UI_FRAMEBUFFER_HEIGHT; ++i) {
        ctx->framebuffer[i] = color;
    }
    pthread_mutex_unlock(&ctx->fb_lock);
}

void ui_context_fill_rect(ui_context_t *ctx, int x, int y, int width, int height,
                          ui_color_t color)
{
    if (!ctx || width <= 0 || height <= 0) {
        return;
    }
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + width;
    int y1 = y + height;
    if (x1 > UI_FRAMEBUFFER_WIDTH) {
        x1 = UI_FRAMEBUFFER_WIDTH;
    }
    if (y1 > UI_FRAMEBUFFER_HEIGHT) {
        y1 = UI_FRAMEBUFFER_HEIGHT;
    }

    pthread_mutex_lock(&ctx->fb_lock);
    for (int row = y0; row < y1; ++row) {
        ui_color_t *base = &ctx->framebuffer[row * UI_FRAMEBUFFER_WIDTH + x0];
        for (int col = x0; col < x1; ++col) {
            *base++ = color;
        }
    }
    pthread_mutex_unlock(&ctx->fb_lock);
}

void ui_context_set_pixel(ui_context_t *ctx, int x, int y, ui_color_t color)
{
    if (!ctx) {
        return;
    }
    pthread_mutex_lock(&ctx->fb_lock);
    ui_set_pixel_locked(ctx, x, y, color);
    pthread_mutex_unlock(&ctx->fb_lock);
}

void ui_context_draw_codepoint(ui_context_t *ctx, int x, int y, uint32_t codepoint,
                               ui_color_t color)
{
    if (!ctx) {
        return;
    }

    bareui_font_glyph_t glyph;
    if (!ui_context_get_glyph(ctx, codepoint, &glyph)) {
        return;
    }

    pthread_mutex_lock(&ctx->fb_lock);
    ui_draw_glyph_locked(ctx, x, y, &glyph, color);
    pthread_mutex_unlock(&ctx->fb_lock);
}

void ui_context_draw_text(ui_context_t *ctx, int x, int y, const char *text,
                          ui_color_t color)
{
    if (!ctx || !text) {
        return;
    }

    int cursor = x;
    int baseline = y;
    const char *ptr = text;
    int line_height = (ctx->font ? ctx->font->height : BAREUI_FONT_HEIGHT) + 1;

    while (*ptr) {
        uint32_t codepoint;
        if (!ui_next_codepoint(&ptr, &codepoint)) {
            break;
        }
        if (codepoint == '\n') {
            cursor = x;
            baseline += line_height;
            continue;
        }

        bareui_font_glyph_t glyph;
        if (!ui_context_get_glyph(ctx, codepoint, &glyph)) {
            continue;
        }
        pthread_mutex_lock(&ctx->fb_lock);
        ui_draw_glyph_locked(ctx, cursor, baseline, &glyph, color);
        pthread_mutex_unlock(&ctx->fb_lock);
        cursor += glyph.spacing;
    }
}

bool ui_context_poll_event(ui_context_t *ctx, ui_event_t *event)
{
    if (!ctx || !event) {
        return false;
    }
    pthread_mutex_lock(&ctx->ev_lock);
    if (ctx->ev_count == 0) {
        pthread_mutex_unlock(&ctx->ev_lock);
        return false;
    }
    *event = ctx->ev_queue[ctx->ev_head];
    ctx->ev_head = (ctx->ev_head + 1) % UI_EVENT_QUEUE_SIZE;
    ctx->ev_count--;
    pthread_mutex_unlock(&ctx->ev_lock);
    return true;
}

bool ui_context_post_event(ui_context_t *ctx, const ui_event_t *event)
{
    if (!ctx || !event) {
        return false;
    }
    pthread_mutex_lock(&ctx->ev_lock);
    if (ctx->ev_count == UI_EVENT_QUEUE_SIZE) {
        pthread_mutex_unlock(&ctx->ev_lock);
        return false;
    }
    size_t insert = (ctx->ev_head + ctx->ev_count) % UI_EVENT_QUEUE_SIZE;
    ctx->ev_queue[insert] = *event;
    ctx->ev_count++;
    pthread_mutex_unlock(&ctx->ev_lock);
    return true;
}

void ui_context_render(ui_context_t *ctx)
{
    if (!ctx || !ctx->hal || !ctx->hal->commit_frame) {
        return;
    }
    pthread_mutex_lock(&ctx->fb_lock);
    ctx->hal->commit_frame(ctx, ctx->framebuffer);
    pthread_mutex_unlock(&ctx->fb_lock);
}

const bareui_font_t *ui_context_font(const ui_context_t *ctx)
{
    return ctx ? ctx->font : NULL;
}

void ui_context_set_font(ui_context_t *ctx, const bareui_font_t *font)
{
    if (ctx) {
        ctx->font = font ? font : bareui_font_default();
    }
}

void *ui_context_user_data(ui_context_t *ctx)
{
    return ctx ? ctx->user_data : NULL;
}

void ui_context_set_user_data(ui_context_t *ctx, void *data)
{
    if (ctx) {
        ctx->user_data = data;
    }
}

const ui_hal_ops_t *ui_context_hal(const ui_context_t *ctx)
{
    return ctx ? ctx->hal : NULL;
}
