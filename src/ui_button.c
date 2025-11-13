#define _POSIX_C_SOURCE 200809L

#include "ui_button.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ui_primitives.h"
#include "ui_font.h"
#include "ui_shadow.h"

#define UI_BUTTON_LONG_PRESS_MS 500

struct ui_button {
    ui_widget_t base;
    char *text;
    const bareui_font_t *font;
    ui_color_t background_color;
    ui_color_t text_color;
    ui_color_t hover_color;
    ui_color_t pressed_color;
    ui_color_t border_color;
    int border_width;
    bool hovered;
    bool pressed;
    bool focused;
    bool autofocus;
    bool enabled;
    bool rtl;
    struct timespec pressed_at;
    bool long_press_fired;
    ui_button_event_fn on_click;
    void *on_click_data;
    ui_button_hover_fn on_hover;
    void *on_hover_data;
    ui_button_event_fn on_focus;
    void *on_focus_data;
    ui_button_event_fn on_blur;
    void *on_blur_data;
    ui_button_event_fn on_long_press;
    void *on_long_press_data;
};

static inline bool ui_button_contains(const ui_button_t *button, int x, int y)
{
    if (!button) {
        return false;
    }
    const ui_rect_t *bounds = &button->base.bounds;
    return x >= bounds->x && y >= bounds->y &&
           x < bounds->x + bounds->width && y < bounds->y + bounds->height;
}

static bool ui_button_next_codepoint(const unsigned char **ptr, uint32_t *out)
{
    if (!ptr || !*ptr || **ptr == '\0') {
        return false;
    }
    unsigned char first = **ptr;
    uint32_t codepoint = first;
    size_t advance = 1;
    if (first < 0x80) {
        // ASCII
    } else if ((first & 0xE0) == 0xC0) {
        codepoint &= 0x1F;
        advance = 2;
    } else if ((first & 0xF0) == 0xE0) {
        codepoint &= 0x0F;
        advance = 3;
    } else if ((first & 0xF8) == 0xF0) {
        codepoint &= 0x07;
        advance = 4;
    } else {
        return false;
    }
    for (size_t i = 1; i < advance; ++i) {
        unsigned char c = (*ptr)[i];
        if ((c & 0xC0) != 0x80) {
            return false;
        }
        codepoint = (codepoint << 6) | (c & 0x3F);
    }
    *ptr += advance;
    if (out) {
        *out = codepoint;
    }
    return true;
}

static int ui_button_measure_text(const ui_button_t *button)
{
    const char *text = button && button->text ? button->text : "";
    const bareui_font_t *font = button && button->font ? button->font : bareui_font_default();
    const unsigned char *ptr = (const unsigned char *)text;
    int width = 0;
    while (*ptr) {
        uint32_t cp;
        if (!ui_button_next_codepoint(&ptr, &cp)) {
            break;
        }
        bareui_font_glyph_t glyph;
        if (bareui_font_lookup(font, cp, &glyph)) {
            width += glyph.spacing;
        }
    }
    return width;
}

static void ui_button_draw_text(ui_context_t *ctx, const ui_button_t *button, int x, int y)
{
    if (!button || !button->text) {
        return;
    }
    const bareui_font_t *font = button->font ? button->font : bareui_font_default();
    const bareui_font_t *prev = ui_context_font(ctx);
    ui_context_set_font(ctx, font);
    ui_context_draw_text(ctx, x, y, button->text, button->text_color);
    ui_context_set_font(ctx, prev);
}

static void ui_button_apply_style(ui_widget_t *widget, const ui_style_t *style)
{
    if (!widget || !style) {
        return;
    }
    ui_button_t *button = (ui_button_t *)widget;
    if (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR) {
        button->background_color = style->background_color;
    }
    if (style->flags & UI_STYLE_FLAG_FOREGROUND_COLOR) {
        button->text_color = style->foreground_color;
    }
    if (style->flags & UI_STYLE_FLAG_ACCENT_COLOR) {
        button->hover_color = style->accent_color;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_COLOR) {
        button->border_color = style->border_color;
        button->pressed_color = style->border_color;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_WIDTH) {
        button->border_width = style->border_width;
    }
}

static void ui_button_notify_hover(ui_button_t *button, bool hover_state)
{
    if (!button || button->hovered == hover_state) {
        return;
    }
    button->hovered = hover_state;
    if (button->on_hover) {
        button->on_hover(button, hover_state, button->on_hover_data);
    }
}

static void ui_button_notify_focus(ui_button_t *button, bool focus_state)
{
    if (!button || button->focused == focus_state) {
        return;
    }
    button->focused = focus_state;
    if (focus_state && button->on_focus) {
        button->on_focus(button, button->on_focus_data);
    } else if (!focus_state && button->on_blur) {
        button->on_blur(button, button->on_blur_data);
    }
}

static void ui_button_try_long_press(ui_button_t *button)
{
    if (!button || button->long_press_fired || !button->pressed) {
        return;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long delta_ms = (now.tv_sec - button->pressed_at.tv_sec) * 1000 +
                    (now.tv_nsec - button->pressed_at.tv_nsec) / 1000000;
    if (delta_ms >= UI_BUTTON_LONG_PRESS_MS) {
        button->long_press_fired = true;
        if (button->on_long_press) {
            button->on_long_press(button, button->on_long_press_data);
        }
    }
}

static ui_color_t ui_button_background(const ui_button_t *button)
{
    if (!button->enabled) {
        return ui_color_from_hex(0x303030);
    }
    if (button->pressed) {
        return button->pressed_color;
    }
    if (button->hovered) {
        return button->hover_color;
    }
    return button->background_color;
}

static bool ui_button_handle_event(ui_widget_t *widget, const ui_event_t *event)
{
    ui_button_t *button = (ui_button_t *)widget;
    if (!button || !button->base.visible || !button->enabled) {
        return false;
    }
    switch (event->type) {
    case UI_EVENT_TOUCH_DOWN: {
        if (!ui_button_contains(button, event->data.touch.x, event->data.touch.y)) {
            return false;
        }
        button->pressed = true;
        clock_gettime(CLOCK_MONOTONIC, &button->pressed_at);
        button->long_press_fired = false;
        ui_button_notify_focus(button, true);
        ui_button_notify_hover(button, true);
        return true;
    }
    case UI_EVENT_TOUCH_MOVE: {
        bool inside = ui_button_contains(button, event->data.touch.x, event->data.touch.y);
        ui_button_notify_hover(button, inside);
        if (!inside && button->pressed) {
            button->pressed = false;
        }
        ui_button_try_long_press(button);
        return inside;
    }
    case UI_EVENT_TOUCH_UP: {
        bool inside = ui_button_contains(button, event->data.touch.x, event->data.touch.y);
        if (button->pressed && inside && button->on_click) {
            button->on_click(button, button->on_click_data);
        }
        button->pressed = false;
        ui_button_notify_hover(button, inside);
        if (!inside) {
            ui_button_notify_focus(button, false);
        }
        return true;
    }
    case UI_EVENT_KEY_DOWN: {
        uint32_t key = event->data.key.keycode;
        if (key == ' ' || key == '\n' || key == '\r') {
            button->pressed = true;
            clock_gettime(CLOCK_MONOTONIC, &button->pressed_at);
            button->long_press_fired = false;
            ui_button_notify_focus(button, true);
            return true;
        }
        break;
    }
    case UI_EVENT_KEY_UP: {
        uint32_t key = event->data.key.keycode;
        if ((key == ' ' || key == '\n' || key == '\r') && button->pressed) {
            button->pressed = false;
            if (button->on_click) {
                button->on_click(button, button->on_click_data);
            }
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

static bool ui_button_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    ui_button_t *button = (ui_button_t *)widget;
    if (!button || !bounds) {
        return false;
    }
    const ui_style_t *style = &button->base.style;
    ui_shadow_t shadow = {
        .enabled = style->shadow_enabled,
        .offset_x = style->shadow_offset_x,
        .offset_y = style->shadow_offset_y,
        .color = style->shadow_color ? style->shadow_color : ui_color_from_hex(0x000000),
        .blur = 0
    };
    ui_shadow_render(ctx, bounds, &shadow);
    ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                         ui_button_background(button));
    int border_width = button->border_width;
    if (style->flags & UI_STYLE_FLAG_BORDER_WIDTH) {
        border_width = style->border_width;
    }
    if (border_width > 0) {
        ui_color_t border = (style->flags & UI_STYLE_FLAG_BORDER_COLOR)
                                ? style->border_color
                                : button->border_color;
        for (int w = 0; w < border_width; ++w) {
            int x = bounds->x + w;
            int y = bounds->y + w;
            int width = bounds->width - w * 2;
            int height = bounds->height - w * 2;
            ui_context_fill_rect(ctx, x, y, width, 1, border);
            ui_context_fill_rect(ctx, x, y + height - 1, width, 1, border);
            ui_context_fill_rect(ctx, x, y, 1, height, border);
            ui_context_fill_rect(ctx, x + width - 1, y, 1, height, border);
        }
    }
    int text_width = ui_button_measure_text(button);
    const bareui_font_t *font = button->font ? button->font : bareui_font_default();
    int text_height = font->height;
    int x = bounds->x + (bounds->width - text_width) / 2;
    if (x < bounds->x) {
        x = bounds->x;
    }
    int y = bounds->y + (bounds->height - text_height) / 2;
    if (y < bounds->y) {
        y = bounds->y;
    }
    if (button->rtl) {
        x = bounds->x + bounds->width - (x - bounds->x) - text_width;
    }
    ui_button_draw_text(ctx, button, x, y);
    return true;
}

static const ui_widget_ops_t ui_button_ops = {
    .render = ui_button_render,
    .handle_event = ui_button_handle_event,
    .destroy = NULL,
    .style_changed = ui_button_apply_style
};

static char *ui_button_copy_text(const char *text)
{
    if (!text) {
        return NULL;
    }
    size_t len = strlen(text) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, text, len);
    }
    return copy;
}

ui_button_t *ui_button_create(void)
{
    ui_button_t *button = calloc(1, sizeof(*button));
    if (!button) {
        return NULL;
    }
    ui_button_init(button);
    return button;
}

void ui_button_destroy(ui_button_t *button)
{
    if (!button) {
        return;
    }
    free(button->text);
    free(button);
}

void ui_button_init(ui_button_t *button)
{
    if (!button) {
        return;
    }
    ui_widget_init(&button->base, &ui_button_ops);
    button->text = NULL;
    button->font = bareui_font_default();
    button->background_color = ui_color_from_hex(0x202020);
    button->text_color = ui_color_from_hex(0xFFFFFF);
    button->hover_color = ui_color_from_hex(0x303030);
    button->pressed_color = ui_color_from_hex(0x101010);
    button->border_color = ui_color_from_hex(0x444444);
    button->border_width = 2;
    button->hovered = false;
    button->pressed = false;
    button->focused = false;
    button->autofocus = false;
    button->enabled = true;
    button->rtl = false;
    button->long_press_fired = false;
    ui_style_t default_style;
    ui_style_init(&default_style);
    default_style.background_color = button->background_color;
    default_style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    default_style.foreground_color = button->text_color;
    default_style.flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    default_style.accent_color = button->hover_color;
    default_style.flags |= UI_STYLE_FLAG_ACCENT_COLOR;
    default_style.border_color = button->border_color;
    default_style.flags |= UI_STYLE_FLAG_BORDER_COLOR;
    default_style.border_width = 2;
    default_style.flags |= UI_STYLE_FLAG_BORDER_WIDTH;
    default_style.shadow_enabled = true;
    default_style.shadow_color = ui_color_from_hex(0x050711);
    default_style.shadow_offset_x = 1;
    default_style.shadow_offset_y = 1;
    default_style.flags |= UI_STYLE_FLAG_SHADOW;
    ui_widget_set_style(&button->base, &default_style);
}

void ui_button_set_text(ui_button_t *button, const char *text)
{
    if (!button) {
        return;
    }
    free(button->text);
    button->text = ui_button_copy_text(text);
}

const char *ui_button_text(const ui_button_t *button)
{
    return button ? button->text : NULL;
}

void ui_button_set_font(ui_button_t *button, const bareui_font_t *font)
{
    if (button) {
        button->font = font ? font : bareui_font_default();
    }
}

const bareui_font_t *ui_button_font(const ui_button_t *button)
{
    return button ? button->font : NULL;
}

void ui_button_set_background_color(ui_button_t *button, ui_color_t color)
{
    if (!button) {
        return;
    }
    button->background_color = color;
    ui_style_t *style = &button->base.style;
    style->background_color = color;
    style->flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    if (button->base.ops && button->base.ops->style_changed) {
        button->base.ops->style_changed(&button->base, style);
    }
}

ui_color_t ui_button_background_color(const ui_button_t *button)
{
    return button ? button->background_color : 0;
}

void ui_button_set_text_color(ui_button_t *button, ui_color_t color)
{
    if (!button) {
        return;
    }
    button->text_color = color;
    ui_style_t *style = &button->base.style;
    style->foreground_color = color;
    style->flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    if (button->base.ops && button->base.ops->style_changed) {
        button->base.ops->style_changed(&button->base, style);
    }
}

ui_color_t ui_button_text_color(const ui_button_t *button)
{
    return button ? button->text_color : 0;
}

void ui_button_set_hover_color(ui_button_t *button, ui_color_t color)
{
    if (!button) {
        return;
    }
    button->hover_color = color;
    ui_style_t *style = &button->base.style;
    style->accent_color = color;
    style->flags |= UI_STYLE_FLAG_ACCENT_COLOR;
    if (button->base.ops && button->base.ops->style_changed) {
        button->base.ops->style_changed(&button->base, style);
    }
}

ui_color_t ui_button_hover_color(const ui_button_t *button)
{
    return button ? button->hover_color : 0;
}

void ui_button_set_pressed_color(ui_button_t *button, ui_color_t color)
{
    if (!button) {
        return;
    }
    button->pressed_color = color;
    ui_style_t *style = &button->base.style;
    style->border_color = color;
    style->flags |= UI_STYLE_FLAG_BORDER_COLOR;
    if (button->base.ops && button->base.ops->style_changed) {
        button->base.ops->style_changed(&button->base, style);
    }
}

ui_color_t ui_button_pressed_color(const ui_button_t *button)
{
    return button ? button->pressed_color : 0;
}

void ui_button_set_autofocus(ui_button_t *button, bool autofocus)
{
    if (button) {
        button->autofocus = autofocus;
    }
}

bool ui_button_autofocus(const ui_button_t *button)
{
    return button ? button->autofocus : false;
}

void ui_button_set_rtl(ui_button_t *button, bool rtl)
{
    if (button) {
        button->rtl = rtl;
    }
}

bool ui_button_rtl(const ui_button_t *button)
{
    return button ? button->rtl : false;
}

void ui_button_set_enabled(ui_button_t *button, bool enabled)
{
    if (button) {
        button->enabled = enabled;
    }
}

bool ui_button_enabled(const ui_button_t *button)
{
    return button ? button->enabled : false;
}

void ui_button_set_on_click(ui_button_t *button, ui_button_event_fn handler, void *user_data)
{
    if (button) {
        button->on_click = handler;
        button->on_click_data = user_data;
    }
}

void ui_button_set_on_hover(ui_button_t *button, ui_button_hover_fn handler, void *user_data)
{
    if (button) {
        button->on_hover = handler;
        button->on_hover_data = user_data;
    }
}

void ui_button_set_on_focus(ui_button_t *button, ui_button_event_fn handler, void *user_data)
{
    if (button) {
        button->on_focus = handler;
        button->on_focus_data = user_data;
    }
}

void ui_button_set_on_blur(ui_button_t *button, ui_button_event_fn handler, void *user_data)
{
    if (button) {
        button->on_blur = handler;
        button->on_blur_data = user_data;
    }
}

void ui_button_set_on_long_press(ui_button_t *button, ui_button_event_fn handler, void *user_data)
{
    if (button) {
        button->on_long_press = handler;
        button->on_long_press_data = user_data;
    }
}

const ui_widget_t *ui_button_widget(const ui_button_t *button)
{
    return button ? &button->base : NULL;
}

ui_widget_t *ui_button_widget_mutable(ui_button_t *button)
{
    return button ? &button->base : NULL;
}
