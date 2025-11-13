#include "ui_switch.h"

#include "ui_primitives.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ui_switch {
    ui_widget_t base;
    char *label;
    const bareui_font_t *font;
    bool value;
    bool hovered;
    bool pressed;
    bool focused;
    bool enabled;
    bool autofocus;
    bool adaptive;
    ui_color_t active_color;
    ui_color_t active_track_color;
    ui_color_t inactive_track_color;
    ui_color_t inactive_thumb_color;
    ui_color_t hover_color;
    ui_color_t focus_color;
    ui_color_t thumb_color;
    ui_color_t track_color;
    ui_color_t track_outline_color;
    int track_outline_width;
    ui_switch_label_position_t label_position;
    ui_switch_change_fn on_change;
    void *on_change_data;
    ui_switch_event_fn on_focus;
    void *on_focus_data;
    ui_switch_event_fn on_blur;
    void *on_blur_data;
};

static const ui_widget_ops_t ui_switch_ops;

static bool ui_switch_contains(const ui_switch_impl_t *sw, int x, int y)
{
    if (!sw) {
        return false;
    }
    const ui_rect_t *bounds = &sw->base.bounds;
    return x >= bounds->x && y >= bounds->y &&
           x < bounds->x + bounds->width && y < bounds->y + bounds->height;
}

static bool ui_switch_next_codepoint(const unsigned char **ptr, uint32_t *out)
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

static int ui_switch_measure_label(const ui_switch_impl_t *sw)
{
    if (!sw || !sw->label || !*sw->label) {
        return 0;
    }
    const bareui_font_t *font = sw->font ? sw->font : bareui_font_default();
    const unsigned char *ptr = (const unsigned char *)sw->label;
    int width = 0;
    while (*ptr) {
        uint32_t cp;
        if (!ui_switch_next_codepoint(&ptr, &cp)) {
            break;
        }
        bareui_font_glyph_t glyph;
        if (bareui_font_lookup(font, cp, &glyph)) {
            width += glyph.spacing;
        }
    }
    return width;
}

static char *ui_switch_copy_label(const char *label)
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

static ui_color_t ui_switch_track_fill_color(const ui_switch_impl_t *sw)
{
    if (!sw) {
        return 0;
    }
    if (!sw->enabled) {
        if (sw->inactive_track_color) {
            return sw->inactive_track_color;
        }
        return sw->track_color;
    }
    if (sw->hovered && sw->hover_color) {
        return sw->hover_color;
    }
    if (sw->value) {
        return sw->active_track_color ? sw->active_track_color : sw->track_color;
    }
    return sw->inactive_track_color ? sw->inactive_track_color : sw->track_color;
}

static ui_color_t ui_switch_thumb_fill_color(const ui_switch_impl_t *sw)
{
    if (!sw) {
        return 0;
    }
    if (!sw->enabled) {
        if (sw->inactive_thumb_color) {
            return sw->inactive_thumb_color;
        }
        return sw->thumb_color;
    }
    if (sw->value && sw->active_color) {
        return sw->active_color;
    }
    if (!sw->value && sw->inactive_thumb_color) {
        return sw->inactive_thumb_color;
    }
    return sw->thumb_color;
}

static void ui_switch_notify_hover(ui_switch_impl_t *sw, bool hovering)
{
    if (!sw || sw->hovered == hovering) {
        return;
    }
    sw->hovered = hovering;
}

static void ui_switch_notify_focus(ui_switch_impl_t *sw, bool focused)
{
    if (!sw || sw->focused == focused) {
        return;
    }
    sw->focused = focused;
    if (focused && sw->on_focus) {
        sw->on_focus((ui_switch_t *)sw, sw->on_focus_data);
    } else if (!focused && sw->on_blur) {
        sw->on_blur((ui_switch_t *)sw, sw->on_blur_data);
    }
}

static void ui_switch_fill_circle(ui_context_t *ctx, int cx, int cy, int radius, ui_color_t color)
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

static void ui_switch_draw_label(ui_context_t *ctx, const ui_switch_impl_t *sw, int x, int y,
                                 ui_color_t color)
{
    if (!ctx || !sw || !sw->label || !*sw->label) {
        return;
    }
    const bareui_font_t *font = sw->font ? sw->font : bareui_font_default();
    const bareui_font_t *prev = ui_context_font(ctx);
    ui_context_set_font(ctx, font);
    ui_context_draw_text(ctx, x, y, sw->label, color);
    ui_context_set_font(ctx, prev);
}

static void ui_switch_apply_style(ui_widget_t *widget, const ui_style_t *style)
{
    if (!widget || !style) {
        return;
    }
    ui_switch_impl_t *sw = (ui_switch_impl_t *)widget;
    if (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR) {
        sw->track_color = style->background_color;
    }
    if (style->flags & UI_STYLE_FLAG_FOREGROUND_COLOR) {
        sw->thumb_color = style->foreground_color;
    }
    if (style->flags & UI_STYLE_FLAG_ACCENT_COLOR) {
        sw->active_color = style->accent_color;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_COLOR) {
        sw->track_outline_color = style->border_color;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_WIDTH) {
        sw->track_outline_width = style->border_width;
    }
}

static bool ui_switch_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    ui_switch_impl_t *sw = (ui_switch_impl_t *)widget;
    if (!sw || !bounds || bounds->width <= 0 || bounds->height <= 0) {
        return false;
    }
    const int margin = 6;
    const int label_gap = sw->label ? 8 : 0;
    int label_width = ui_switch_measure_label(sw);
    int track_height = bounds->height - margin;
    if (track_height < 8) {
        track_height = bounds->height - 2;
    }
    if (track_height < 6) {
        track_height = 6;
    }
    int min_track_width = track_height * 2;
    int available_width = bounds->width - margin * 2 -
                          (label_width > 0 ? label_gap + label_width : 0);
    int track_width = available_width > min_track_width ? available_width : min_track_width;
    int track_y = bounds->y + (bounds->height - track_height) / 2;
    int track_x = bounds->x + margin;
    int label_x = bounds->x + margin;
    if (sw->label && sw->label_position == UI_SWITCH_LABEL_POSITION_LEFT) {
        label_x = bounds->x + margin;
        track_x = label_x + label_width + (label_width > 0 ? label_gap : 0);
    } else {
        track_x = bounds->x + margin;
        label_x = track_x + track_width + (label_width > 0 ? label_gap : 0);
    }
    int label_y = bounds->y + (bounds->height - (sw->font ? sw->font->height : bareui_font_default()->height)) / 2;

    ui_color_t fill_color = ui_switch_track_fill_color(sw);
    if (sw->focused && sw->focus_color) {
        int focus_padding = 3;
        ui_context_fill_rect(ctx, track_x - focus_padding, track_y - focus_padding,
                             track_width + focus_padding * 2, track_height + focus_padding * 2,
                             sw->focus_color);
    }

    if (sw->track_outline_width > 0 && sw->track_outline_color) {
        ui_context_fill_rect(ctx, track_x, track_y, track_width, track_height,
                             sw->track_outline_color);
        int inner_x = track_x + sw->track_outline_width;
        int inner_y = track_y + sw->track_outline_width;
        int inner_w = track_width - sw->track_outline_width * 2;
        int inner_h = track_height - sw->track_outline_width * 2;
        if (inner_w > 0 && inner_h > 0) {
            ui_context_fill_rect(ctx, inner_x, inner_y, inner_w, inner_h, fill_color);
        }
    } else {
        ui_context_fill_rect(ctx, track_x, track_y, track_width, track_height, fill_color);
    }

    int thumb_radius = track_height / 2 - 1;
    if (thumb_radius < 3) {
        thumb_radius = track_height / 2;
    }
    if (thumb_radius < 2) {
        thumb_radius = 2;
    }
    int thumb_center_y = track_y + track_height / 2;
    int thumb_center_x = track_x + (sw->value ? track_width - thumb_radius - 2 : thumb_radius + 2);
    ui_color_t thumb_color = ui_switch_thumb_fill_color(sw);
    ui_switch_fill_circle(ctx, thumb_center_x, thumb_center_y, thumb_radius, thumb_color);

    if (sw->label && *sw->label) {
        ui_switch_draw_label(ctx, sw, label_x, label_y, sw->thumb_color);
    }
    return true;
}

static bool ui_switch_handle_event(ui_widget_t *widget, const ui_event_t *event)
{
    ui_switch_impl_t *sw = (ui_switch_impl_t *)widget;
    if (!sw || !event) {
        return false;
    }
    switch (event->type) {
    case UI_EVENT_TOUCH_DOWN: {
        if (!sw->enabled) {
            return false;
        }
        if (!ui_switch_contains(sw, event->data.touch.x, event->data.touch.y)) {
            ui_switch_notify_focus(sw, false);
            return false;
        }
        sw->pressed = true;
        ui_switch_notify_focus(sw, true);
        ui_switch_notify_hover(sw, true);
        return true;
    }
    case UI_EVENT_TOUCH_MOVE: {
        if (!sw->enabled) {
            return false;
        }
        bool inside = ui_switch_contains(sw, event->data.touch.x, event->data.touch.y);
        ui_switch_notify_hover(sw, inside);
        return sw->pressed;
    }
    case UI_EVENT_TOUCH_UP: {
        if (!sw->enabled) {
            return false;
        }
        if (!sw->pressed) {
            return false;
        }
        bool inside = ui_switch_contains(sw, event->data.touch.x, event->data.touch.y);
        sw->pressed = false;
        ui_switch_notify_hover(sw, inside);
        if (inside) {
            ui_switch_set_value((ui_switch_t *)sw, !sw->value);
        }
        return true;
    }
    case UI_EVENT_KEY_DOWN: {
        if (!sw->enabled || !sw->focused) {
            return false;
        }
        uint32_t keycode = event->data.key.keycode;
        if (keycode == ' ' || keycode == '\n' || keycode == '\r') {
            ui_switch_set_value((ui_switch_t *)sw, !sw->value);
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

static const ui_widget_ops_t ui_switch_ops = {
    .render = ui_switch_render,
    .handle_event = ui_switch_handle_event,
    .destroy = NULL,
    .style_changed = ui_switch_apply_style
};

ui_switch_t *ui_switch_create(void)
{
    ui_switch_impl_t *sw = calloc(1, sizeof(*sw));
    if (!sw) {
        return NULL;
    }
    ui_switch_init((ui_switch_t *)sw);
    return (ui_switch_t *)sw;
}

void ui_switch_destroy(ui_switch_t *sw)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    free(impl->label);
    free(impl);
}

void ui_switch_init(ui_switch_t *sw)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    memset(impl, 0, sizeof(*impl));
    ui_widget_init(&impl->base, &ui_switch_ops);
    impl->font = bareui_font_default();
    impl->value = false;
    impl->hovered = false;
    impl->pressed = false;
    impl->focused = false;
    impl->enabled = true;
    impl->autofocus = false;
    impl->adaptive = false;
    impl->active_color = ui_color_from_hex(0xFFFFFF);
    impl->active_track_color = ui_color_from_hex(0x25C06B);
    impl->inactive_track_color = ui_color_from_hex(0x5D5D5D);
    impl->inactive_thumb_color = ui_color_from_hex(0xFFFFFF);
    impl->hover_color = ui_color_from_hex(0x98C6FF);
    impl->focus_color = ui_color_from_hex(0x6DA3FF);
    impl->thumb_color = ui_color_from_hex(0xFFFFFF);
    impl->track_color = ui_color_from_hex(0x888888);
    impl->track_outline_color = ui_color_from_hex(0x0A0A10);
    impl->track_outline_width = 1;
    impl->label_position = UI_SWITCH_LABEL_POSITION_RIGHT;
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = impl->track_color;
    style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    style.foreground_color = impl->thumb_color;
    style.flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    style.accent_color = impl->active_color;
    style.flags |= UI_STYLE_FLAG_ACCENT_COLOR;
    style.border_color = impl->track_outline_color;
    style.border_width = impl->track_outline_width;
    style.border_sides = UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM;
    style.flags |= UI_STYLE_FLAG_BORDER_COLOR | UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BORDER_SIDES;
    ui_widget_set_style(&impl->base, &style);
}

void ui_switch_set_value(ui_switch_t *sw, bool value)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    if (impl->value == value) {
        return;
    }
    impl->value = value;
    if (impl->on_change) {
        impl->on_change(sw, value, impl->on_change_data);
    }
}

bool ui_switch_value(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->value : false;
}

void ui_switch_set_enabled(ui_switch_t *sw, bool enabled)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->enabled = enabled;
    if (!enabled) {
        impl->pressed = false;
        ui_switch_notify_hover(impl, false);
        ui_switch_notify_focus(impl, false);
    }
}

bool ui_switch_enabled(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->enabled : false;
}

void ui_switch_set_active_color(ui_switch_t *sw, ui_color_t color)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->active_color = color;
    ui_style_t *style = &impl->base.style;
    style->accent_color = color;
    style->flags |= UI_STYLE_FLAG_ACCENT_COLOR;
    if (impl->base.ops && impl->base.ops->style_changed) {
        impl->base.ops->style_changed(&impl->base, style);
    }
}

ui_color_t ui_switch_active_color(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->active_color : 0;
}

void ui_switch_set_active_track_color(ui_switch_t *sw, ui_color_t color)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->active_track_color = color;
    ui_style_t *style = &impl->base.style;
    style->flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    if (impl->base.ops && impl->base.ops->style_changed) {
        impl->base.ops->style_changed(&impl->base, style);
    }
}

ui_color_t ui_switch_active_track_color(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->active_track_color : 0;
}

void ui_switch_set_inactive_track_color(ui_switch_t *sw, ui_color_t color)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->inactive_track_color = color;
    ui_style_t *style = &impl->base.style;
    style->flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    if (impl->base.ops && impl->base.ops->style_changed) {
        impl->base.ops->style_changed(&impl->base, style);
    }
}

ui_color_t ui_switch_inactive_track_color(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->inactive_track_color : 0;
}

void ui_switch_set_inactive_thumb_color(ui_switch_t *sw, ui_color_t color)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->inactive_thumb_color = color;
    ui_style_t *style = &impl->base.style;
    style->flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    if (impl->base.ops && impl->base.ops->style_changed) {
        impl->base.ops->style_changed(&impl->base, style);
    }
}

ui_color_t ui_switch_inactive_thumb_color(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->inactive_thumb_color : 0;
}

void ui_switch_set_hover_color(ui_switch_t *sw, ui_color_t color)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->hover_color = color;
}

ui_color_t ui_switch_hover_color(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->hover_color : 0;
}

void ui_switch_set_focus_color(ui_switch_t *sw, ui_color_t color)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->focus_color = color;
}

ui_color_t ui_switch_focus_color(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->focus_color : 0;
}

void ui_switch_set_track_outline_color(ui_switch_t *sw, ui_color_t color)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->track_outline_color = color;
    ui_style_t *style = &impl->base.style;
    style->border_color = color;
    style->flags |= UI_STYLE_FLAG_BORDER_COLOR;
    if (impl->base.ops && impl->base.ops->style_changed) {
        impl->base.ops->style_changed(&impl->base, style);
    }
}

ui_color_t ui_switch_track_outline_color(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->track_outline_color : 0;
}

void ui_switch_set_track_outline_width(ui_switch_t *sw, int width)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->track_outline_width = width >= 0 ? width : 0;
    ui_style_t *style = &impl->base.style;
    style->border_width = impl->track_outline_width;
    style->flags |= UI_STYLE_FLAG_BORDER_WIDTH;
    if (impl->base.ops && impl->base.ops->style_changed) {
        impl->base.ops->style_changed(&impl->base, style);
    }
}

int ui_switch_track_outline_width(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->track_outline_width : 0;
}

void ui_switch_set_label(ui_switch_t *sw, const char *label)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    free(impl->label);
    impl->label = ui_switch_copy_label(label);
}

const char *ui_switch_label(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->label : NULL;
}

void ui_switch_set_label_position(ui_switch_t *sw, ui_switch_label_position_t position)
{
    if (!sw) {
        return;
    }
    ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
    impl->label_position = position;
}

ui_switch_label_position_t ui_switch_label_position(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->label_position : UI_SWITCH_LABEL_POSITION_RIGHT;
}

void ui_switch_set_font(ui_switch_t *sw, const bareui_font_t *font)
{
    if (sw) {
        ((ui_switch_impl_t *)sw)->font = font ? font : bareui_font_default();
    }
}

const bareui_font_t *ui_switch_font(const ui_switch_t *sw)
{
    return sw ? ((const ui_switch_impl_t *)sw)->font : NULL;
}

void ui_switch_set_on_change(ui_switch_t *sw, ui_switch_change_fn handler, void *user_data)
{
    if (sw) {
        ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
        impl->on_change = handler;
        impl->on_change_data = user_data;
    }
}

void ui_switch_set_on_focus(ui_switch_t *sw, ui_switch_event_fn handler, void *user_data)
{
    if (sw) {
        ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
        impl->on_focus = handler;
        impl->on_focus_data = user_data;
    }
}

void ui_switch_set_on_blur(ui_switch_t *sw, ui_switch_event_fn handler, void *user_data)
{
    if (sw) {
        ui_switch_impl_t *impl = (ui_switch_impl_t *)sw;
        impl->on_blur = handler;
        impl->on_blur_data = user_data;
    }
}

const ui_widget_t *ui_switch_widget(const ui_switch_t *sw)
{
    return sw ? &((const ui_switch_impl_t *)sw)->base : NULL;
}

ui_widget_t *ui_switch_widget_mutable(ui_switch_t *sw)
{
    return sw ? &((ui_switch_impl_t *)sw)->base : NULL;
}
