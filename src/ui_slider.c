#include "ui_slider.h"

#include "ui_font.h"
#include "ui_primitives.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UI_SLIDER_TRACK_MARGIN 6
#define UI_SLIDER_TRACK_HEIGHT 4
#define UI_SLIDER_THUMB_RADIUS 6
#define UI_SLIDER_LABEL_PADDING 4
#define UI_SLIDER_LABEL_BUFFER 128

struct ui_slider {
    ui_widget_t base;
    double min;
    double max;
    double value;
    double secondary_value;
    int divisions;
    int value_round;
    ui_color_t active_color;
    ui_color_t inactive_color;
    ui_color_t thumb_color;
    ui_color_t overlay_color;
    ui_color_t secondary_active_color;
    ui_slider_interaction_t interaction;
    ui_mouse_cursor_t mouse_cursor;
    bool autofocus;
    bool adaptive;
    bool dragging;
    bool focused;
    bool change_in_progress;
    bool has_secondary_value;
    char *label_format;
    ui_slider_event_fn on_focus;
    void *on_focus_data;
    ui_slider_event_fn on_blur;
    void *on_blur_data;
    ui_slider_value_event_fn on_change;
    void *on_change_data;
    ui_slider_value_event_fn on_change_start;
    void *on_change_start_data;
    ui_slider_value_event_fn on_change_end;
    void *on_change_end_data;
};

typedef struct {
    int track_x;
    int track_y;
    int track_width;
    int track_height;
    int thumb_center_x;
    int thumb_center_y;
    double ratio;
} ui_slider_layout_t;

static const ui_widget_ops_t ui_slider_ops;

static bool ui_slider_next_codepoint(const unsigned char **ptr, uint32_t *out)
{
    if (!ptr || !*ptr || **ptr == '\0') {
        return false;
    }
    unsigned char first = **ptr;
    uint32_t cp = first;
    size_t advance = 1;
    if (first < 0x80) {
    } else if ((first & 0xE0) == 0xC0) {
        cp &= 0x1F;
        advance = 2;
    } else if ((first & 0xF0) == 0xE0) {
        cp &= 0x0F;
        advance = 3;
    } else if ((first & 0xF8) == 0xF0) {
        cp &= 0x07;
        advance = 4;
    } else {
        return false;
    }
    for (size_t i = 1; i < advance; ++i) {
        unsigned char c = (*ptr)[i];
        if ((c & 0xC0) != 0x80) {
            return false;
        }
        cp = (cp << 6) | (c & 0x3F);
    }
    *ptr += advance;
    if (out) {
        *out = cp;
    }
    return true;
}

static int ui_slider_measure_text(const bareui_font_t *font, const char *text)
{
    if (!text || !font) {
        return 0;
    }
    int width = 0;
    const unsigned char *ptr = (const unsigned char *)text;
    while (*ptr) {
        uint32_t cp = 0;
        if (!ui_slider_next_codepoint(&ptr, &cp)) {
            break;
        }
        bareui_font_glyph_t glyph;
        if (bareui_font_lookup(font, cp, &glyph)) {
            width += glyph.spacing;
        }
    }
    return width;
}

static char *ui_slider_strdup(const char *src)
{
    if (!src) {
        return NULL;
    }
    size_t len = strlen(src) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, src, len);
    }
    return copy;
}

static const ui_style_t *ui_widget_style_safe(const ui_widget_t *widget)
{
    static const ui_style_t default_style = {0};
    const ui_style_t *style = ui_widget_style(widget);
    return style ? style : &default_style;
}

static bool ui_slider_enabled(const ui_slider_t *slider)
{
    if (!slider) {
        return false;
    }
    return slider->max > slider->min;
}

static void ui_slider_notify_focus(ui_slider_t *slider, bool focus_state)
{
    if (!slider || slider->focused == focus_state) {
        return;
    }
    slider->focused = focus_state;
    if (focus_state && slider->on_focus) {
        slider->on_focus(slider, slider->on_focus_data);
    } else if (!focus_state && slider->on_blur) {
        slider->on_blur(slider, slider->on_blur_data);
    }
}

static double ui_slider_round_value(double value, int decimals)
{
    if (decimals <= 0) {
        if (value >= 0.0) {
            return floor(value + 0.5);
        }
        return ceil(value - 0.5);
    }
    double factor = 1.0;
    for (int i = 0; i < decimals; ++i) {
        factor *= 10.0;
    }
    double scaled = value * factor;
    if (scaled >= 0.0) {
        scaled = floor(scaled + 0.5);
    } else {
        scaled = ceil(scaled - 0.5);
    }
    return scaled / factor;
}

static double ui_slider_quantize_value(const ui_slider_t *slider, double value)
{
    if (!slider) {
        return value;
    }
    double min = slider->min;
    double max = slider->max;
    if (max <= min) {
        return min;
    }
    if (slider->divisions <= 0) {
        if (value < min) {
            return min;
        }
        if (value > max) {
            return max;
        }
        return value;
    }
    double span = max - min;
    double step = span / slider->divisions;
    if (step <= 0.0) {
        return min;
    }
    double offset = value - min;
    double index = offset / step;
    double rounded = floor(index + 0.5);
    double result = min + rounded * step;
    if (result < min) {
        result = min;
    }
    if (result > max) {
        result = max;
    }
    return result;
}

static double ui_slider_value_for_position(const ui_slider_t *slider, const ui_rect_t *bounds,
                                           int x)
{
    if (!slider || !bounds) {
        return 0;
    }
    ui_slider_layout_t layout = {0};
    const ui_style_t *style = ui_widget_style_safe(&slider->base);
    layout.track_height = UI_SLIDER_TRACK_HEIGHT;
    int padding_x = style->padding_left + style->padding_right;
    int available_width = bounds->width - padding_x - UI_SLIDER_TRACK_MARGIN * 2;
    layout.track_width = available_width > 0 ? available_width : 0;
    layout.track_x = bounds->x + style->padding_left + UI_SLIDER_TRACK_MARGIN;
    if (layout.track_width < 0) {
        layout.track_width = 0;
    }
    double span = slider->max - slider->min;
    if (span <= 0.0 || layout.track_width <= 0) {
        return slider->value;
    }
    int clamped_x = x;
    int max_x = layout.track_x + layout.track_width;
    if (clamped_x < layout.track_x) {
        clamped_x = layout.track_x;
    }
    if (clamped_x > max_x) {
        clamped_x = max_x;
    }
    double ratio = layout.track_width > 0 ? (double)(clamped_x - layout.track_x) / layout.track_width : 0.0;
    double target = slider->min + ratio * span;
    return ui_slider_quantize_value(slider, target);
}

static ui_slider_layout_t ui_slider_compute_layout(const ui_slider_t *slider,
                                                   const ui_rect_t *bounds)
{
    ui_slider_layout_t layout = {0};
    if (!slider || !bounds) {
        return layout;
    }
    const ui_style_t *style = ui_widget_style_safe(&slider->base);
    int padding_x = style->padding_left + style->padding_right;
    int padding_y = style->padding_top + style->padding_bottom;
    int inner_height = bounds->height - padding_y;
    if (inner_height < UI_SLIDER_TRACK_HEIGHT) {
        inner_height = UI_SLIDER_TRACK_HEIGHT;
    }
    layout.track_height = UI_SLIDER_TRACK_HEIGHT;
    layout.track_y = bounds->y + style->padding_top + (inner_height - layout.track_height) / 2;
    layout.track_x = bounds->x + style->padding_left + UI_SLIDER_TRACK_MARGIN;
    int available_width = bounds->width - padding_x - UI_SLIDER_TRACK_MARGIN * 2;
    layout.track_width = available_width > 0 ? available_width : 0;
    double span = slider->max - slider->min;
    double ratio = 0.0;
    if (span > 0.0) {
        ratio = (slider->value - slider->min) / span;
        if (ratio < 0.0) {
            ratio = 0.0;
        } else if (ratio > 1.0) {
            ratio = 1.0;
        }
    }
    layout.ratio = ratio;
    layout.thumb_center_x = layout.track_x + (int)(ratio * layout.track_width);
    layout.thumb_center_y = layout.track_y + layout.track_height / 2;
    return layout;
}

static void ui_slider_fill_circle(ui_context_t *ctx, int cx, int cy, int radius, ui_color_t color)
{
    if (!ctx || radius <= 0) {
        return;
    }
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy <= radius * radius) {
                ui_context_set_pixel(ctx, cx + dx, cy + dy, color);
            }
        }
    }
}

static void ui_slider_build_label(const ui_slider_t *slider, char *out, size_t size)
{
    if (!slider || !slider->label_format || !out || size == 0) {
        if (size > 0) {
            out[0] = '\0';
        }
        return;
    }
    int decimals = slider->value_round >= 0 ? slider->value_round : 0;
    if (decimals > 6) {
        decimals = 6;
    }
    double shown_value = ui_slider_round_value(slider->value, decimals);
    char value_buffer[32];
    snprintf(value_buffer, sizeof(value_buffer), "%.*f", decimals, shown_value);
    const char *placeholder = "{value}";
    const char *format = slider->label_format;
    const char *pos = strstr(format, placeholder);
    if (!pos) {
        strncpy(out, format, size - 1);
        out[size - 1] = '\0';
        return;
    }
    size_t prefix_len = pos - format;
    size_t copy_len = prefix_len < size - 1 ? prefix_len : size - 1;
    memcpy(out, format, copy_len);
    size_t written = copy_len;
    const char *suffix = pos + strlen(placeholder);
    size_t remaining = size - written;
    size_t value_len = strlen(value_buffer);
    size_t to_copy = value_len < remaining - 1 ? value_len : (remaining > 0 ? remaining - 1 : 0);
    if (to_copy > 0) {
        memcpy(out + written, value_buffer, to_copy);
        written += to_copy;
    }
    remaining = size - written;
    if (remaining > 1) {
        size_t suffix_len = strlen(suffix);
        size_t suffix_copy = suffix_len < remaining - 1 ? suffix_len : remaining - 1;
        memcpy(out + written, suffix, suffix_copy);
        written += suffix_copy;
    }
    if (written < size) {
        out[written] = '\0';
    } else {
        out[size - 1] = '\0';
    }
}

static void ui_slider_draw_value_indicator(ui_context_t *ctx, const ui_slider_t *slider,
                                          const ui_slider_layout_t *layout,
                                          const ui_rect_t *bounds)
{
    if (!ctx || !slider || !layout || !bounds || !slider->label_format) {
        return;
    }
    if (!slider->dragging && !slider->focused) {
        return;
    }
    char label_text[UI_SLIDER_LABEL_BUFFER];
    ui_slider_build_label(slider, label_text, sizeof(label_text));
    if (!label_text[0]) {
        return;
    }
    const bareui_font_t *font = bareui_font_default();
    int text_width = ui_slider_measure_text(font, label_text);
    int text_height = font->height;
    int indicator_width = text_width + UI_SLIDER_LABEL_PADDING * 2;
    int indicator_height = text_height + UI_SLIDER_LABEL_PADDING * 2;
    int indicator_x = layout->thumb_center_x - indicator_width / 2;
    int indicator_y = layout->track_y - indicator_height - 6;
    if (indicator_x < bounds->x) {
        indicator_x = bounds->x;
    }
    int max_x = bounds->x + bounds->width - indicator_width;
    if (max_x < bounds->x) {
        max_x = bounds->x;
    }
    if (indicator_x > max_x) {
        indicator_x = max_x;
    }
    if (indicator_y < bounds->y) {
        indicator_y = bounds->y;
    }
    ui_context_fill_rect(ctx, indicator_x, indicator_y, indicator_width, indicator_height,
                         slider->thumb_color);
    const bareui_font_t *prev = ui_context_font(ctx);
    ui_context_set_font(ctx, font);
    ui_context_draw_text(ctx, indicator_x + UI_SLIDER_LABEL_PADDING,
                         indicator_y + UI_SLIDER_LABEL_PADDING, label_text,
                         slider->overlay_color ? slider->overlay_color : slider->thumb_color);
    ui_context_set_font(ctx, prev);
}

static void ui_slider_notify_change(ui_slider_t *slider, double value)
{
    if (!slider) {
        return;
    }
    double clamped = ui_slider_quantize_value(slider, value);
    if (clamped == slider->value) {
        return;
    }
    slider->value = clamped;
    if (slider->on_change) {
        slider->on_change(slider, slider->value, slider->on_change_data);
    }
}

static void ui_slider_begin_interaction(ui_slider_t *slider)
{
    if (!slider) {
        return;
    }
    if (!slider->change_in_progress) {
        slider->change_in_progress = true;
        if (slider->on_change_start) {
            slider->on_change_start(slider, slider->value, slider->on_change_start_data);
        }
    }
}

static void ui_slider_end_interaction(ui_slider_t *slider)
{
    if (!slider) {
        return;
    }
    if (slider->change_in_progress) {
        slider->change_in_progress = false;
        if (slider->on_change_end) {
            slider->on_change_end(slider, slider->value, slider->on_change_end_data);
        }
    }
}

static bool ui_slider_hit_test(const ui_slider_t *slider, int x, int y)
{
    if (!slider) {
        return false;
    }
    const ui_rect_t *bounds = &slider->base.bounds;
    return x >= bounds->x && y >= bounds->y &&
           x < bounds->x + bounds->width && y < bounds->y + bounds->height;
}

static bool ui_slider_hit_thumb(const ui_slider_t *slider, int x, int y, const ui_rect_t *bounds)
{
    if (!slider || !bounds) {
        return false;
    }
    ui_slider_layout_t layout = ui_slider_compute_layout(slider, bounds);
    int dx = x - layout.thumb_center_x;
    int dy = y - layout.thumb_center_y;
    int radius = UI_SLIDER_THUMB_RADIUS + 4;
    return dx * dx + dy * dy <= radius * radius;
}

static bool ui_slider_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    if (!ctx || !widget || !bounds) {
        return false;
    }
    ui_slider_t *slider = (ui_slider_t *)widget;
    const ui_style_t *style = ui_widget_style_safe(widget);
    if (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR) {
        ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                             style->background_color);
    }
    ui_slider_layout_t layout = ui_slider_compute_layout(slider, bounds);
    ui_color_t inactive = slider->inactive_color;
    ui_color_t active = slider->active_color;
    ui_color_t thumb = slider->thumb_color;
    if (!ui_slider_enabled(slider)) {
        inactive = ui_color_from_hex(0x444444);
        active = ui_color_from_hex(0x666666);
        thumb = ui_color_from_hex(0x383838);
    }
    if (layout.track_width > 0) {
        ui_context_fill_rect(ctx, layout.track_x, layout.track_y, layout.track_width,
                             layout.track_height, inactive);
        int active_width = (int)(layout.ratio * layout.track_width);
        if (active_width > 0) {
            ui_context_fill_rect(ctx, layout.track_x, layout.track_y, active_width,
                                 layout.track_height, active);
        }
        if (slider->has_secondary_value && slider->secondary_active_color != 0 && slider->max > slider->min) {
            double span = slider->max - slider->min;
            double ratio = (slider->secondary_value - slider->min) / span;
            if (ratio > layout.ratio && ratio <= 1.0) {
                int start = layout.track_x + active_width;
                int sec_width = (int)((ratio - layout.ratio) * layout.track_width);
                if (sec_width > 0) {
                    ui_context_fill_rect(ctx, start, layout.track_y, sec_width,
                                         layout.track_height, slider->secondary_active_color);
                }
            }
        }
    }
    if (slider->overlay_color && (slider->dragging || slider->focused)) {
        ui_slider_fill_circle(ctx, layout.thumb_center_x, layout.thumb_center_y,
                              UI_SLIDER_THUMB_RADIUS + 2, slider->overlay_color);
    }
    ui_slider_fill_circle(ctx, layout.thumb_center_x, layout.thumb_center_y, UI_SLIDER_THUMB_RADIUS,
                          thumb);
    ui_slider_draw_value_indicator(ctx, slider, &layout, bounds);
    return true;
}

static bool ui_slider_handle_event(ui_widget_t *widget, const ui_event_t *event)
{
    ui_slider_t *slider = (ui_slider_t *)widget;
    if (!slider || !event || !slider->base.visible || !ui_slider_enabled(slider)) {
        return false;
    }
    const ui_rect_t *bounds = &slider->base.bounds;
    switch (event->type) {
    case UI_EVENT_TOUCH_DOWN: {
        if (!ui_slider_hit_test(slider, event->data.touch.x, event->data.touch.y)) {
            return false;
        }
        if (slider->interaction == UI_SLIDER_INTERACTION_DRAG_ONLY &&
            !ui_slider_hit_thumb(slider, event->data.touch.x, event->data.touch.y, bounds)) {
            return false;
        }
        slider->dragging = true;
        ui_slider_begin_interaction(slider);
        ui_slider_notify_focus(slider, true);
        double new_value = ui_slider_value_for_position(slider, bounds, event->data.touch.x);
        ui_slider_notify_change(slider, new_value);
        return true;
    }
    case UI_EVENT_TOUCH_MOVE:
        if (!slider->dragging) {
            return false;
        }
        ui_slider_notify_change(slider,
                                ui_slider_value_for_position(slider, bounds, event->data.touch.x));
        return true;
    case UI_EVENT_TOUCH_UP:
        if (!slider->dragging) {
            return false;
        }
        slider->dragging = false;
        ui_slider_notify_change(slider,
                                ui_slider_value_for_position(slider, bounds, event->data.touch.x));
        ui_slider_end_interaction(slider);
        ui_slider_notify_focus(slider, false);
        return true;
    default:
        return false;
    }
}

static void ui_slider_destroy_internal(ui_widget_t *widget)
{
    ui_slider_t *slider = (ui_slider_t *)widget;
    if (!slider) {
        return;
    }
    if (slider->label_format) {
        free(slider->label_format);
        slider->label_format = NULL;
    }
}

static void ui_slider_apply_style(ui_widget_t *widget, const ui_style_t *style)
{
    if (!widget || !style) {
        return;
    }
    ui_slider_t *slider = (ui_slider_t *)widget;
    if (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR) {
        slider->inactive_color = style->background_color;
    }
    if (style->flags & UI_STYLE_FLAG_FOREGROUND_COLOR) {
        slider->thumb_color = style->foreground_color;
    }
    if (style->flags & UI_STYLE_FLAG_ACCENT_COLOR) {
        slider->active_color = style->accent_color;
    }
}

static const ui_widget_ops_t ui_slider_ops = {
    .render = ui_slider_render,
    .handle_event = ui_slider_handle_event,
    .destroy = ui_slider_destroy_internal,
    .style_changed = ui_slider_apply_style
};

ui_slider_t *ui_slider_create(void)
{
    ui_slider_t *slider = calloc(1, sizeof(*slider));
    if (!slider) {
        return NULL;
    }
    ui_slider_init(slider);
    return slider;
}

void ui_slider_destroy(ui_slider_t *slider)
{
    if (!slider) {
        return;
    }
    ui_slider_destroy_internal(&slider->base);
    free(slider);
}

void ui_slider_init(ui_slider_t *slider)
{
    if (!slider) {
        return;
    }
    memset(slider, 0, sizeof(*slider));
    slider->min = 0.0;
    slider->max = 1.0;
    slider->value = 0.0;
    slider->active_color = ui_color_from_hex(0x5A9ADB);
    slider->inactive_color = ui_color_from_hex(0x2E2E3C);
    slider->thumb_color = ui_color_from_hex(0xF2F2F2);
    slider->overlay_color = ui_color_from_hex(0x4040F0);
    slider->secondary_active_color = ui_color_from_hex(0x6FB9FF);
    slider->interaction = UI_SLIDER_INTERACTION_TAP_AND_SLIDE;
    slider->mouse_cursor = UI_MOUSE_CURSOR_DEFAULT;
    slider->value_round = 0;
    slider->divisions = 0;
    slider->autofocus = false;
    slider->adaptive = false;
    slider->dragging = false;
    slider->focused = false;
    slider->change_in_progress = false;
    slider->has_secondary_value = false;
    slider->label_format = NULL;
    ui_widget_init(&slider->base, &ui_slider_ops);
}

void ui_slider_set_value(ui_slider_t *slider, double value)
{
    if (!slider) {
        return;
    }
    slider->value = ui_slider_quantize_value(slider, value);
}

double ui_slider_value(const ui_slider_t *slider)
{
    return slider ? slider->value : 0.0;
}

void ui_slider_set_min(ui_slider_t *slider, double min)
{
    if (!slider) {
        return;
    }
    slider->min = min;
    if (slider->max < slider->min) {
        slider->max = slider->min;
    }
    if (slider->value < slider->min) {
        slider->value = slider->min;
    }
    if (slider->has_secondary_value && slider->secondary_value < slider->min) {
        slider->secondary_value = slider->min;
    }
}

double ui_slider_min(const ui_slider_t *slider)
{
    return slider ? slider->min : 0.0;
}

void ui_slider_set_max(ui_slider_t *slider, double max)
{
    if (!slider) {
        return;
    }
    slider->max = max;
    if (slider->min > slider->max) {
        slider->min = slider->max;
    }
    if (slider->value > slider->max) {
        slider->value = slider->max;
    }
    if (slider->has_secondary_value && slider->secondary_value > slider->max) {
        slider->secondary_value = slider->max;
    }
}

double ui_slider_max(const ui_slider_t *slider)
{
    return slider ? slider->max : 0.0;
}

void ui_slider_set_divisions(ui_slider_t *slider, int divisions)
{
    if (!slider) {
        return;
    }
    if (divisions < 0) {
        divisions = 0;
    }
    slider->divisions = divisions;
    slider->value = ui_slider_quantize_value(slider, slider->value);
}

int ui_slider_divisions(const ui_slider_t *slider)
{
    return slider ? slider->divisions : 0;
}

void ui_slider_set_active_color(ui_slider_t *slider, ui_color_t color)
{
    if (slider) {
        slider->active_color = color;
    }
}

ui_color_t ui_slider_active_color(const ui_slider_t *slider)
{
    return slider ? slider->active_color : 0;
}

void ui_slider_set_inactive_color(ui_slider_t *slider, ui_color_t color)
{
    if (slider) {
        slider->inactive_color = color;
    }
}

ui_color_t ui_slider_inactive_color(const ui_slider_t *slider)
{
    return slider ? slider->inactive_color : 0;
}

void ui_slider_set_thumb_color(ui_slider_t *slider, ui_color_t color)
{
    if (slider) {
        slider->thumb_color = color;
    }
}

ui_color_t ui_slider_thumb_color(const ui_slider_t *slider)
{
    return slider ? slider->thumb_color : 0;
}

void ui_slider_set_overlay_color(ui_slider_t *slider, ui_color_t color)
{
    if (slider) {
        slider->overlay_color = color;
    }
}

ui_color_t ui_slider_overlay_color(const ui_slider_t *slider)
{
    return slider ? slider->overlay_color : 0;
}

void ui_slider_set_secondary_active_color(ui_slider_t *slider, ui_color_t color)
{
    if (slider) {
        slider->secondary_active_color = color;
    }
}

ui_color_t ui_slider_secondary_active_color(const ui_slider_t *slider)
{
    return slider ? slider->secondary_active_color : 0;
}

void ui_slider_set_secondary_track_value(ui_slider_t *slider, double value)
{
    if (!slider) {
        return;
    }
    if (value < slider->min) {
        value = slider->min;
    }
    if (value > slider->max) {
        value = slider->max;
    }
    slider->secondary_value = value;
    slider->has_secondary_value = true;
}

double ui_slider_secondary_track_value(const ui_slider_t *slider)
{
    if (!slider) {
        return 0.0;
    }
    return slider->secondary_value;
}

bool ui_slider_has_secondary_track_value(const ui_slider_t *slider)
{
    return slider ? slider->has_secondary_value : false;
}

void ui_slider_clear_secondary_track_value(ui_slider_t *slider)
{
    if (slider) {
        slider->has_secondary_value = false;
    }
}

void ui_slider_set_label(ui_slider_t *slider, const char *label)
{
    if (!slider) {
        return;
    }
    if (slider->label_format) {
        free(slider->label_format);
        slider->label_format = NULL;
    }
    if (label) {
        slider->label_format = ui_slider_strdup(label);
    }
}

const char *ui_slider_label(const ui_slider_t *slider)
{
    return slider ? slider->label_format : NULL;
}

void ui_slider_set_round(ui_slider_t *slider, int decimals)
{
    if (slider) {
        slider->value_round = decimals;
    }
}

int ui_slider_round(const ui_slider_t *slider)
{
    return slider ? slider->value_round : 0;
}

void ui_slider_set_interaction(ui_slider_t *slider, ui_slider_interaction_t interaction)
{
    if (slider) {
        slider->interaction = interaction;
    }
}

ui_slider_interaction_t ui_slider_interaction(const ui_slider_t *slider)
{
    return slider ? slider->interaction : UI_SLIDER_INTERACTION_TAP_AND_SLIDE;
}

void ui_slider_set_autofocus(ui_slider_t *slider, bool autofocus)
{
    if (slider) {
        slider->autofocus = autofocus;
    }
}

bool ui_slider_autofocus(const ui_slider_t *slider)
{
    return slider ? slider->autofocus : false;
}

void ui_slider_set_adaptive(ui_slider_t *slider, bool adaptive)
{
    if (slider) {
        slider->adaptive = adaptive;
    }
}

bool ui_slider_adaptive(const ui_slider_t *slider)
{
    return slider ? slider->adaptive : false;
}

void ui_slider_set_mouse_cursor(ui_slider_t *slider, ui_mouse_cursor_t cursor)
{
    if (slider) {
        slider->mouse_cursor = cursor;
    }
}

ui_mouse_cursor_t ui_slider_mouse_cursor(const ui_slider_t *slider)
{
    return slider ? slider->mouse_cursor : UI_MOUSE_CURSOR_DEFAULT;
}

void ui_slider_set_on_change(ui_slider_t *slider, ui_slider_value_event_fn handler,
                             void *user_data)
{
    if (slider) {
        slider->on_change = handler;
        slider->on_change_data = user_data;
    }
}

void ui_slider_set_on_change_start(ui_slider_t *slider, ui_slider_value_event_fn handler,
                                   void *user_data)
{
    if (slider) {
        slider->on_change_start = handler;
        slider->on_change_start_data = user_data;
    }
}

void ui_slider_set_on_change_end(ui_slider_t *slider, ui_slider_value_event_fn handler,
                                 void *user_data)
{
    if (slider) {
        slider->on_change_end = handler;
        slider->on_change_end_data = user_data;
    }
}

void ui_slider_set_on_focus(ui_slider_t *slider, ui_slider_event_fn handler, void *user_data)
{
    if (slider) {
        slider->on_focus = handler;
        slider->on_focus_data = user_data;
    }
}

void ui_slider_set_on_blur(ui_slider_t *slider, ui_slider_event_fn handler, void *user_data)
{
    if (slider) {
        slider->on_blur = handler;
        slider->on_blur_data = user_data;
    }
}

const ui_widget_t *ui_slider_widget(const ui_slider_t *slider)
{
    return slider ? &slider->base : NULL;
}

ui_widget_t *ui_slider_widget_mutable(ui_slider_t *slider)
{
    return slider ? &slider->base : NULL;
}
