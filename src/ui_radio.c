#include "ui_radio.h"

#include "ui_primitives.h"

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define UI_RADIO_DEFAULT_ACTIVE_COLOR ui_color_from_hex(0x1E88E5)
#define UI_RADIO_DEFAULT_FILL_COLOR ui_color_from_hex(0xFFFFFF)
#define UI_RADIO_DEFAULT_BORDER_COLOR ui_color_from_hex(0x757575)
#define UI_RADIO_DEFAULT_BORDER_COLOR_CUPERTINO ui_color_from_hex(0x8E8E93)
#define UI_RADIO_DEFAULT_HOVER_COLOR ui_color_from_hex(0xBDBDBD)
#define UI_RADIO_DEFAULT_FOCUS_COLOR ui_color_from_hex(0x64B5F6)
#define UI_RADIO_DEFAULT_OVERLAY_COLOR ui_color_from_hex(0x448AFF)
#define UI_RADIO_DEFAULT_LABEL_COLOR ui_color_from_hex(0x111111)
#define UI_RADIO_DEFAULT_SPRING_RADIUS 16

struct ui_radio {
    ui_widget_t base;
    char *label;
    const bareui_font_t *label_font;
    ui_radio_group_t *group;
    int32_t value;
    bool selected;
    bool hovered;
    bool pressed;
    bool focused;
    bool enabled;
    bool autofocus;
    bool adaptive;
    bool toggleable;
    ui_radio_label_position_t label_position;
    ui_radio_visual_density_t visual_density;
    ui_color_t active_color;
    ui_color_t fill_color;
    ui_color_t border_color;
    ui_color_t hover_color;
    ui_color_t focus_color;
    ui_color_t overlay_color;
    ui_color_t label_color;
    ui_mouse_cursor_t mouse_cursor;
    int splash_radius;
    ui_radio_change_fn on_change;
    void *on_change_data;
    ui_radio_event_fn on_focus;
    void *on_focus_data;
    ui_radio_event_fn on_blur;
    void *on_blur_data;
};

struct ui_radio_group {
    int32_t value;
    ui_radio_t **radios;
    size_t radio_count;
    size_t radio_capacity;
    ui_radio_group_change_fn on_change;
    void *on_change_data;
};

static const ui_widget_ops_t ui_radio_ops;
static void ui_radio_destroy_impl(ui_widget_t *widget);

static bool ui_radio_platform_is_cupertino(void)
{
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

static bool ui_radio_contains(const ui_radio_t *radio, int x, int y)
{
    if (!radio) {
        return false;
    }
    const ui_rect_t *bounds = &radio->base.bounds;
    return x >= bounds->x && y >= bounds->y && x < bounds->x + bounds->width &&
           y < bounds->y + bounds->height;
}

static bool ui_radio_next_codepoint(const unsigned char **ptr, uint32_t *out)
{
    if (!ptr || !*ptr || **ptr == '\0') {
        return false;
    }
    unsigned char first = **ptr;
    uint32_t codepoint = first;
    size_t advance = 1;
    if (first < 0x80) {
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

static int ui_radio_measure_label(const ui_radio_t *radio, const char *text)
{
    if (!radio || !text) {
        return 0;
    }
    const bareui_font_t *font = radio->label_font ? radio->label_font : bareui_font_default();
    const unsigned char *ptr = (const unsigned char *)text;
    int width = 0;
    while (*ptr) {
        uint32_t codepoint;
        if (!ui_radio_next_codepoint(&ptr, &codepoint)) {
            break;
        }
        bareui_font_glyph_t glyph;
        if (bareui_font_lookup(font, codepoint, &glyph)) {
            width += glyph.spacing;
        }
    }
    return width;
}

static char *ui_radio_copy_label(const char *label)
{
    if (!label) {
        return NULL;
    }
    size_t len = strlen(label) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, label, len);
    }
    return copy;
}

static void ui_radio_fill_circle(ui_context_t *ctx, int cx, int cy, int radius, ui_color_t color)
{
    if (!ctx || radius <= 0) {
        return;
    }
    int radius_sq = radius * radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        int y = cy + dy;
        int limit = radius_sq - dy * dy;
        if (limit < 0) {
            continue;
        }
        int span = (int)(sqrt((double)limit));
        for (int dx = -span; dx <= span; ++dx) {
            ui_context_set_pixel(ctx, cx + dx, y, color);
        }
    }
}

static void ui_radio_notify_hover(ui_radio_t *radio, bool hovering)
{
    if (!radio) {
        return;
    }
    radio->hovered = hovering;
}

static void ui_radio_notify_focus(ui_radio_t *radio, bool focus_state)
{
    if (!radio || radio->focused == focus_state) {
        return;
    }
    radio->focused = focus_state;
    if (focus_state && radio->on_focus) {
        radio->on_focus(radio, radio->on_focus_data);
    } else if (!focus_state && radio->on_blur) {
        radio->on_blur(radio, radio->on_blur_data);
    }
}

static void ui_radio_set_selected_internal(ui_radio_t *radio, bool selected, bool notify)
{
    if (!radio || radio->selected == selected) {
        return;
    }
    radio->selected = selected;
    if (notify && radio->on_change) {
        radio->on_change(radio, selected ? radio->value : UI_RADIO_VALUE_NONE,
                         radio->on_change_data);
    }
}

static void ui_radio_group_register_radio(ui_radio_group_t *group, ui_radio_t *radio)
{
    if (!group || !radio) {
        return;
    }
    for (size_t i = 0; i < group->radio_count; ++i) {
        if (group->radios[i] == radio) {
            return;
        }
    }
    size_t target = group->radio_count + 1;
    if (target > group->radio_capacity) {
        size_t next = group->radio_capacity ? group->radio_capacity * 2 : 4;
        if (next < target) {
            next = target;
        }
        ui_radio_t **realloced = realloc(group->radios, next * sizeof(*group->radios));
        if (!realloced) {
            return;
        }
        group->radios = realloced;
        group->radio_capacity = next;
    }
    group->radios[group->radio_count++] = radio;
    bool should_select = group->value != UI_RADIO_VALUE_NONE && radio->value == group->value;
    ui_radio_set_selected_internal(radio, should_select, false);
}

static void ui_radio_group_unregister_radio(ui_radio_group_t *group, ui_radio_t *radio)
{
    if (!group || !radio || !group->radios) {
        return;
    }
    size_t idx = group->radio_count;
    for (size_t i = 0; i < group->radio_count; ++i) {
        if (group->radios[i] == radio) {
            idx = i;
            break;
        }
    }
    if (idx == group->radio_count) {
        return;
    }
    for (size_t i = idx + 1; i < group->radio_count; ++i) {
        group->radios[i - 1] = group->radios[i];
    }
    group->radio_count--;
    radio->group = NULL;
}

static void ui_radio_group_apply_value(ui_radio_group_t *group)
{
    if (!group) {
        return;
    }
    for (size_t i = 0; i < group->radio_count; ++i) {
        ui_radio_t *radio = group->radios[i];
        bool should_select = group->value != UI_RADIO_VALUE_NONE && radio->value == group->value;
        ui_radio_set_selected_internal(radio, should_select, true);
    }
}

ui_radio_t *ui_radio_create(void)
{
    ui_radio_t *radio = calloc(1, sizeof(*radio));
    if (!radio) {
        return NULL;
    }
    ui_radio_init(radio);
    return radio;
}

void ui_radio_destroy(ui_radio_t *radio)
{
    if (!radio) {
        return;
    }
    ui_radio_destroy_impl(&radio->base);
    radio->on_change = NULL;
    radio->on_focus = NULL;
    radio->on_blur = NULL;
    free(radio);
}

void ui_radio_init(ui_radio_t *radio)
{
    if (!radio) {
        return;
    }
    ui_widget_init(&radio->base, &ui_radio_ops);
    radio->label = NULL;
    radio->label_font = bareui_font_default();
    radio->group = NULL;
    radio->value = UI_RADIO_VALUE_NONE;
    radio->selected = false;
    radio->hovered = false;
    radio->pressed = false;
    radio->focused = false;
    radio->enabled = true;
    radio->autofocus = false;
    radio->adaptive = false;
    radio->toggleable = false;
    radio->label_position = UI_RADIO_LABEL_POSITION_RIGHT;
    radio->visual_density = UI_RADIO_VISUAL_DENSITY_COMFORTABLE;
    radio->active_color = UI_RADIO_DEFAULT_ACTIVE_COLOR;
    radio->fill_color = UI_RADIO_DEFAULT_FILL_COLOR;
    radio->border_color = UI_RADIO_DEFAULT_BORDER_COLOR;
    radio->hover_color = UI_RADIO_DEFAULT_HOVER_COLOR;
    radio->focus_color = UI_RADIO_DEFAULT_FOCUS_COLOR;
    radio->overlay_color = UI_RADIO_DEFAULT_OVERLAY_COLOR;
    radio->label_color = UI_RADIO_DEFAULT_LABEL_COLOR;
    radio->mouse_cursor = UI_MOUSE_CURSOR_DEFAULT;
    radio->splash_radius = UI_RADIO_DEFAULT_SPRING_RADIUS;
    radio->on_change = NULL;
    radio->on_change_data = NULL;
    radio->on_focus = NULL;
    radio->on_focus_data = NULL;
    radio->on_blur = NULL;
    radio->on_blur_data = NULL;
}

void ui_radio_set_value(ui_radio_t *radio, int32_t value)
{
    if (!radio) {
        return;
    }
    radio->value = value;
    if (radio->group && radio->group->value == value) {
        ui_radio_set_selected_internal(radio, true, false);
    }
}

int32_t ui_radio_value(const ui_radio_t *radio)
{
    return radio ? radio->value : UI_RADIO_VALUE_NONE;
}

void ui_radio_set_selected(ui_radio_t *radio, bool selected)
{
    if (!radio) {
        return;
    }
    if (radio->group) {
        if (selected) {
            ui_radio_group_set_value(radio->group, radio->value);
        } else if (radio->toggleable) {
            ui_radio_group_set_value(radio->group, UI_RADIO_VALUE_NONE);
        }
        return;
    }
    ui_radio_set_selected_internal(radio, selected, true);
}

bool ui_radio_selected(const ui_radio_t *radio)
{
    return radio ? radio->selected : false;
}

void ui_radio_set_group(ui_radio_t *radio, ui_radio_group_t *group)
{
    if (!radio) {
        return;
    }
    if (radio->group == group) {
        return;
    }
    if (radio->group) {
        ui_radio_group_unregister_radio(radio->group, radio);
    }
    if (group) {
        radio->group = group;
        ui_radio_group_register_radio(group, radio);
    } else {
        radio->group = NULL;
    }
}

ui_radio_group_t *ui_radio_group(const ui_radio_t *radio)
{
    return radio ? radio->group : NULL;
}

void ui_radio_set_enabled(ui_radio_t *radio, bool enabled)
{
    if (radio) {
        radio->enabled = enabled;
    }
}

bool ui_radio_enabled(const ui_radio_t *radio)
{
    return radio ? radio->enabled : false;
}

void ui_radio_set_toggleable(ui_radio_t *radio, bool toggleable)
{
    if (radio) {
        radio->toggleable = toggleable;
    }
}

bool ui_radio_toggleable(const ui_radio_t *radio)
{
    return radio ? radio->toggleable : false;
}

void ui_radio_set_autofocus(ui_radio_t *radio, bool autofocus)
{
    if (!radio) {
        return;
    }
    radio->autofocus = autofocus;
    if (autofocus) {
        ui_radio_notify_focus(radio, true);
    }
}

bool ui_radio_autofocus(const ui_radio_t *radio)
{
    return radio ? radio->autofocus : false;
}

void ui_radio_set_adaptive(ui_radio_t *radio, bool adaptive)
{
    if (radio) {
        radio->adaptive = adaptive;
    }
}

bool ui_radio_adaptive(const ui_radio_t *radio)
{
    return radio ? radio->adaptive : false;
}

void ui_radio_set_active_color(ui_radio_t *radio, ui_color_t color)
{
    if (radio) {
        radio->active_color = color;
    }
}

ui_color_t ui_radio_active_color(const ui_radio_t *radio)
{
    return radio ? radio->active_color : UI_RADIO_DEFAULT_ACTIVE_COLOR;
}

void ui_radio_set_fill_color(ui_radio_t *radio, ui_color_t color)
{
    if (radio) {
        radio->fill_color = color;
    }
}

ui_color_t ui_radio_fill_color(const ui_radio_t *radio)
{
    return radio ? radio->fill_color : UI_RADIO_DEFAULT_FILL_COLOR;
}

void ui_radio_set_border_color(ui_radio_t *radio, ui_color_t color)
{
    if (radio) {
        radio->border_color = color;
    }
}

ui_color_t ui_radio_border_color(const ui_radio_t *radio)
{
    return radio ? radio->border_color : UI_RADIO_DEFAULT_BORDER_COLOR;
}

void ui_radio_set_hover_color(ui_radio_t *radio, ui_color_t color)
{
    if (radio) {
        radio->hover_color = color;
    }
}

ui_color_t ui_radio_hover_color(const ui_radio_t *radio)
{
    return radio ? radio->hover_color : UI_RADIO_DEFAULT_HOVER_COLOR;
}

void ui_radio_set_focus_color(ui_radio_t *radio, ui_color_t color)
{
    if (radio) {
        radio->focus_color = color;
    }
}

ui_color_t ui_radio_focus_color(const ui_radio_t *radio)
{
    return radio ? radio->focus_color : UI_RADIO_DEFAULT_FOCUS_COLOR;
}

void ui_radio_set_overlay_color(ui_radio_t *radio, ui_color_t color)
{
    if (radio) {
        radio->overlay_color = color;
    }
}

ui_color_t ui_radio_overlay_color(const ui_radio_t *radio)
{
    return radio ? radio->overlay_color : UI_RADIO_DEFAULT_OVERLAY_COLOR;
}

void ui_radio_set_splash_radius(ui_radio_t *radio, int radius)
{
    if (radio && radius > 0) {
        radio->splash_radius = radius;
    }
}

int ui_radio_splash_radius(const ui_radio_t *radio)
{
    return radio ? radio->splash_radius : UI_RADIO_DEFAULT_SPRING_RADIUS;
}

void ui_radio_set_label(ui_radio_t *radio, const char *label)
{
    if (!radio) {
        return;
    }
    char *copy = ui_radio_copy_label(label);
    if (!copy && label) {
        return;
    }
    free(radio->label);
    radio->label = copy;
}

const char *ui_radio_label(const ui_radio_t *radio)
{
    return radio ? radio->label : NULL;
}

void ui_radio_set_label_font(ui_radio_t *radio, const bareui_font_t *font)
{
    if (radio) {
        radio->label_font = font ? font : bareui_font_default();
    }
}

const bareui_font_t *ui_radio_label_font(const ui_radio_t *radio)
{
    return radio && radio->label_font ? radio->label_font : bareui_font_default();
}

void ui_radio_set_label_color(ui_radio_t *radio, ui_color_t color)
{
    if (radio) {
        radio->label_color = color;
    }
}

ui_color_t ui_radio_label_color(const ui_radio_t *radio)
{
    return radio ? radio->label_color : UI_RADIO_DEFAULT_LABEL_COLOR;
}

void ui_radio_set_label_position(ui_radio_t *radio, ui_radio_label_position_t position)
{
    if (radio) {
        radio->label_position = position;
    }
}

ui_radio_label_position_t ui_radio_label_position(const ui_radio_t *radio)
{
    return radio ? radio->label_position : UI_RADIO_LABEL_POSITION_RIGHT;
}

void ui_radio_set_visual_density(ui_radio_t *radio, ui_radio_visual_density_t density)
{
    if (radio) {
        radio->visual_density = density;
    }
}

ui_radio_visual_density_t ui_radio_visual_density(const ui_radio_t *radio)
{
    return radio ? radio->visual_density : UI_RADIO_VISUAL_DENSITY_COMFORTABLE;
}

void ui_radio_set_mouse_cursor(ui_radio_t *radio, ui_mouse_cursor_t cursor)
{
    if (radio) {
        radio->mouse_cursor = cursor;
    }
}

ui_mouse_cursor_t ui_radio_mouse_cursor(const ui_radio_t *radio)
{
    return radio ? radio->mouse_cursor : UI_MOUSE_CURSOR_DEFAULT;
}

void ui_radio_set_on_change(ui_radio_t *radio, ui_radio_change_fn handler, void *user_data)
{
    if (radio) {
        radio->on_change = handler;
        radio->on_change_data = user_data;
    }
}

void ui_radio_set_on_focus(ui_radio_t *radio, ui_radio_event_fn handler, void *user_data)
{
    if (radio) {
        radio->on_focus = handler;
        radio->on_focus_data = user_data;
    }
}

void ui_radio_set_on_blur(ui_radio_t *radio, ui_radio_event_fn handler, void *user_data)
{
    if (radio) {
        radio->on_blur = handler;
        radio->on_blur_data = user_data;
    }
}

const ui_widget_t *ui_radio_widget(const ui_radio_t *radio)
{
    return radio ? &radio->base : NULL;
}

ui_widget_t *ui_radio_widget_mutable(ui_radio_t *radio)
{
    return radio ? &radio->base : NULL;
}

ui_radio_group_t *ui_radio_group_create(void)
{
    ui_radio_group_t *group = calloc(1, sizeof(*group));
    if (group) {
        group->value = UI_RADIO_VALUE_NONE;
    }
    return group;
}

void ui_radio_group_destroy(ui_radio_group_t *group)
{
    if (!group) {
        return;
    }
    for (size_t i = 0; i < group->radio_count; ++i) {
        group->radios[i]->group = NULL;
    }
    free(group->radios);
    free(group);
}

void ui_radio_group_set_value(ui_radio_group_t *group, int32_t value)
{
    if (!group || group->value == value) {
        return;
    }
    group->value = value;
    ui_radio_group_apply_value(group);
    if (group->on_change) {
        group->on_change(group, value, group->on_change_data);
    }
}

int32_t ui_radio_group_value(const ui_radio_group_t *group)
{
    return group ? group->value : UI_RADIO_VALUE_NONE;
}

void ui_radio_group_set_on_change(ui_radio_group_t *group, ui_radio_group_change_fn handler,
                                  void *user_data)
{
    if (group) {
        group->on_change = handler;
        group->on_change_data = user_data;
    }
}

static void ui_radio_apply_style(ui_widget_t *widget, const ui_style_t *style)
{
    if (!widget || !style) {
        return;
    }
    ui_radio_t *radio = (ui_radio_t *)widget;
    if (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR) {
        radio->fill_color = style->background_color;
    }
    if (style->flags & UI_STYLE_FLAG_FOREGROUND_COLOR) {
        radio->label_color = style->foreground_color;
    }
    if (style->flags & UI_STYLE_FLAG_ACCENT_COLOR) {
        radio->active_color = style->accent_color;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_COLOR) {
        radio->border_color = style->border_color;
    }
}

static void ui_radio_draw_label(ui_context_t *ctx, const ui_radio_t *radio, int x, int y)
{
    if (!ctx || !radio || !radio->label) {
        return;
    }
    const bareui_font_t *font = radio->label_font ? radio->label_font : bareui_font_default();
    const bareui_font_t *prev = ui_context_font(ctx);
    ui_context_set_font(ctx, font);
    ui_context_draw_text(ctx, x, y, radio->label, radio->label_color);
    ui_context_set_font(ctx, prev);
}

static int ui_radio_effective_circle_radius(const ui_radio_t *radio)
{
    if (!radio) {
        return 0;
    }
    int radius = radio->visual_density == UI_RADIO_VISUAL_DENSITY_COMPACT ? 6 : 8;
    if (radio->adaptive && ui_radio_platform_is_cupertino()) {
        radius = radio->visual_density == UI_RADIO_VISUAL_DENSITY_COMPACT ? 5 : 7;
    }
    return radius;
}

static ui_color_t ui_radio_effective_border_color(const ui_radio_t *radio)
{
    if (!radio) {
        return UI_RADIO_DEFAULT_BORDER_COLOR;
    }
    if (radio->border_color) {
        return radio->border_color;
    }
    if (radio->adaptive && ui_radio_platform_is_cupertino()) {
        return UI_RADIO_DEFAULT_BORDER_COLOR_CUPERTINO;
    }
    return UI_RADIO_DEFAULT_BORDER_COLOR;
}

static ui_color_t ui_radio_effective_fill_color(const ui_radio_t *radio)
{
    if (!radio) {
        return UI_RADIO_DEFAULT_FILL_COLOR;
    }
    if (!radio->enabled) {
        return radio->hover_color ? radio->hover_color : radio->fill_color;
    }
    if (radio->hovered && radio->hover_color) {
        return radio->hover_color;
    }
    return radio->fill_color;
}

static bool ui_radio_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    if (!ctx || !widget || !bounds) {
        return false;
    }
    ui_radio_t *radio = (ui_radio_t *)widget;
    int radius = ui_radio_effective_circle_radius(radio);
    int content_margin = radio->visual_density == UI_RADIO_VISUAL_DENSITY_COMPACT ? 4 : 8;
    int spacing = radio->visual_density == UI_RADIO_VISUAL_DENSITY_COMPACT ? 4 : 8;
    int center_y = bounds->y + bounds->height / 2;
    int circle_x = bounds->x + content_margin + radius;
    int label_width = radio->label ? ui_radio_measure_label(radio, radio->label) : 0;
    int label_x = 0;

    if (radio->label_position == UI_RADIO_LABEL_POSITION_LEFT && label_width > 0) {
        label_x = bounds->x + content_margin;
        circle_x = label_x + label_width + spacing + radius;
    } else {
        if (label_width > 0) {
            label_x = circle_x + radius + spacing;
        }
    }

    ui_color_t fill_color = ui_radio_effective_fill_color(radio);
    ui_color_t border_color = ui_radio_effective_border_color(radio);

    if (radio->pressed && radio->overlay_color) {
        ui_radio_fill_circle(ctx, circle_x, center_y, radio->splash_radius, radio->overlay_color);
    }

    if (radio->focused && radio->focus_color) {
        ui_radio_fill_circle(ctx, circle_x, center_y, radius + 2, radio->focus_color);
    }

    ui_radio_fill_circle(ctx, circle_x, center_y, radius, border_color);
    ui_radio_fill_circle(ctx, circle_x, center_y, radius - 1, fill_color);

    if (radio->selected) {
        int dot_radius = radius - 3;
        if (dot_radius > 0) {
            ui_radio_fill_circle(ctx, circle_x, center_y, dot_radius, radio->active_color);
        }
    }

    if (radio->label && label_width > 0) {
        const bareui_font_t *font = radio->label_font ? radio->label_font : bareui_font_default();
        int label_height = font ? font->height : BAREUI_FONT_HEIGHT;
        int text_y = center_y - label_height / 2;
        if (text_y < bounds->y) {
            text_y = bounds->y;
        }
        ui_radio_draw_label(ctx, radio, label_x, text_y);
    }

    return true;
}

static bool ui_radio_handle_event(ui_widget_t *widget, const ui_event_t *event)
{
    ui_radio_t *radio = (ui_radio_t *)widget;
    if (!radio || !radio->base.visible || !radio->enabled) {
        return false;
    }
    switch (event->type) {
    case UI_EVENT_TOUCH_DOWN: {
        if (!ui_radio_contains(radio, event->data.touch.x, event->data.touch.y)) {
            return false;
        }
        radio->pressed = true;
        ui_radio_notify_hover(radio, true);
        ui_radio_notify_focus(radio, true);
        return true;
    }
    case UI_EVENT_TOUCH_MOVE: {
        bool inside = ui_radio_contains(radio, event->data.touch.x, event->data.touch.y);
        ui_radio_notify_hover(radio, inside);
        if (!inside) {
            radio->pressed = false;
        }
        return radio->pressed;
    }
    case UI_EVENT_TOUCH_UP: {
        if (!radio->pressed) {
            return false;
        }
        radio->pressed = false;
        bool inside = ui_radio_contains(radio, event->data.touch.x, event->data.touch.y);
        ui_radio_notify_hover(radio, inside);
        if (!inside) {
            ui_radio_notify_focus(radio, false);
            return true;
        }
        bool currently_selected = radio->selected;
        if (radio->group) {
            if (currently_selected && radio->toggleable) {
                ui_radio_group_set_value(radio->group, UI_RADIO_VALUE_NONE);
            } else if (!currently_selected) {
                ui_radio_group_set_value(radio->group, radio->value);
            }
        } else {
            if (currently_selected && radio->toggleable) {
                ui_radio_set_selected_internal(radio, false, true);
            } else if (!currently_selected) {
                ui_radio_set_selected_internal(radio, true, true);
            }
        }
        ui_radio_notify_focus(radio, false);
        return true;
    }
    default:
        return false;
    }
}

static void ui_radio_destroy_impl(ui_widget_t *widget)
{
    if (!widget) {
        return;
    }
    ui_radio_t *radio = (ui_radio_t *)widget;
    if (radio->group) {
        ui_radio_group_unregister_radio(radio->group, radio);
    }
    free(radio->label);
    radio->label = NULL;
}

static const ui_widget_ops_t ui_radio_ops = {
    .render = ui_radio_render,
    .handle_event = ui_radio_handle_event,
    .destroy = ui_radio_destroy_impl,
    .style_changed = ui_radio_apply_style
};
