#define _POSIX_C_SOURCE 200809L

#include "ui_progressbar.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ui_primitives.h"
#include "ui_shadow.h"

#define UI_PROGRESSBAR_DEFAULT_TRACK_COLOR ui_color_from_hex(0x2F2F2F)
#define UI_PROGRESSBAR_DEFAULT_INDICATOR_COLOR ui_color_from_hex(0x6EC6FF)
#define UI_PROGRESSBAR_ANIMATION_SPEED 0.6
#define UI_PROGRESSBAR_INDETERMINATE_RATIO 0.35

struct ui_progressbar {
    ui_widget_t base;
    double value;
    bool determinate;
    int bar_height;
    ui_border_radius_t border_radius;
    ui_color_t track_color;
    ui_color_t indicator_color;
    ui_color_t border_color;
    int border_width;
    ui_border_side_t border_sides;
    ui_box_shadow_t box_shadow;
    char *semantics_label;
    char *semantics_value;
    char *tooltip;
    double animation_phase;
    struct timespec last_tick;
};

static char *ui_progressbar_duplicate_text(const char *text)
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

static void ui_progressbar_assign_text(char **slot, const char *text)
{
    if (!slot) {
        return;
    }
    char *duplicate = ui_progressbar_duplicate_text(text);
    free(*slot);
    *slot = duplicate;
}

static int ui_progressbar_effective_radius(const ui_border_radius_t *radius)
{
    if (!radius) {
        return 0;
    }
    int values[4] = {radius->top_left, radius->top_right, radius->bottom_right, radius->bottom_left};
    int effective = values[0];
    for (size_t i = 1; i < 4; ++i) {
        if (values[i] < effective) {
            effective = values[i];
        }
    }
    if (effective < 0) {
        effective = 0;
    }
    return effective;
}

static void ui_progressbar_fill_rounded_rect(ui_context_t *ctx, int x, int y, int width,
                                              int height, int radius, ui_color_t color)
{
    if (!ctx || width <= 0 || height <= 0) {
        return;
    }
    int rr = radius;
    if (rr < 0) {
        rr = 0;
    }
    if (rr > width / 2) {
        rr = width / 2;
    }
    if (rr > height / 2) {
        rr = height / 2;
    }
    if (rr == 0) {
        ui_context_fill_rect(ctx, x, y, width, height, color);
        return;
    }
    double radius_sq = (double)rr * (double)rr;
    int inner_height = height - 2 * rr;
    if (inner_height > 0) {
        ui_context_fill_rect(ctx, x, y + rr, width, inner_height, color);
    }
    for (int row = 0; row < rr; ++row) {
        double dy = (double)(rr - row);
        if (dy < 0.0) {
            dy = 0.0;
        }
        double offset = sqrt(fmax(0.0, radius_sq - dy * dy));
        int left = x + rr - (int)offset;
        int right = x + width - rr + (int)offset;
        if (left < x) {
            left = x;
        }
        if (right > x + width) {
            right = x + width;
        }
        int dest_y = y + row;
        if (right > left) {
            ui_context_fill_rect(ctx, left, dest_y, right - left, 1, color);
        }
        dest_y = y + height - 1 - row;
        if (right > left) {
            ui_context_fill_rect(ctx, left, dest_y, right - left, 1, color);
        }
    }
}

static void ui_progressbar_draw_border(ui_context_t *ctx, const ui_rect_t *rect,
                                       int border_width, ui_border_side_t sides, ui_color_t color)
{
    if (!ctx || !rect || border_width <= 0) {
        return;
    }
    int width = rect->width;
    int height = rect->height;
    int bw = border_width;
    if (bw > height) {
        bw = height;
    }
    if (sides & UI_BORDER_TOP) {
        ui_context_fill_rect(ctx, rect->x, rect->y, width, bw, color);
    }
    if (sides & UI_BORDER_BOTTOM) {
        ui_context_fill_rect(ctx, rect->x, rect->y + height - bw, width, bw, color);
    }
    if (sides & UI_BORDER_LEFT) {
        ui_context_fill_rect(ctx, rect->x, rect->y, bw, height, color);
    }
    if (sides & UI_BORDER_RIGHT) {
        ui_context_fill_rect(ctx, rect->x + width - bw, rect->y, bw, height, color);
    }
}

static void ui_progressbar_update_animation(ui_progressbar_t *progress)
{
    if (!progress) {
        return;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double delta = (double)(now.tv_sec - progress->last_tick.tv_sec) +
                   (double)(now.tv_nsec - progress->last_tick.tv_nsec) / 1e9;
    if (delta < 0.0) {
        delta = 0.0;
    }
    progress->last_tick = now;
    if (progress->determinate) {
        return;
    }
    progress->animation_phase += delta * UI_PROGRESSBAR_ANIMATION_SPEED;
    if (progress->animation_phase >= 1.0) {
        progress->animation_phase = fmod(progress->animation_phase, 1.0);
    }
}

static void ui_progressbar_apply_style(ui_widget_t *widget, const ui_style_t *style)
{
    if (!widget || !style) {
        return;
    }
    ui_progressbar_t *progress = (ui_progressbar_t *)widget;
    if (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR) {
        progress->track_color = style->background_color;
    }
    if (style->flags & UI_STYLE_FLAG_FOREGROUND_COLOR) {
        progress->indicator_color = style->foreground_color;
    }
    if (style->flags & UI_STYLE_FLAG_ACCENT_COLOR) {
        progress->indicator_color = style->accent_color;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_COLOR) {
        progress->border_color = style->border_color;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_WIDTH) {
        progress->border_width = style->border_width;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_SIDES) {
        progress->border_sides = style->border_sides;
    }
    if (style->flags & UI_STYLE_FLAG_BOX_SHADOW) {
        progress->box_shadow = style->box_shadow;
    }
}

static bool ui_progressbar_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    if (!ctx || !widget || !bounds) {
        return false;
    }
    ui_progressbar_t *progress = (ui_progressbar_t *)widget;
    if (progress->box_shadow.enabled) {
        ui_shadow_render(ctx, bounds, &progress->box_shadow);
    }
    int track_height = progress->bar_height;
    if (track_height <= 0) {
        track_height = 4;
    }
    if (track_height > bounds->height) {
        track_height = bounds->height;
    }
    int track_width = bounds->width;
    if (track_width <= 0 || track_height <= 0) {
        return true;
    }
    int track_x = bounds->x;
    int track_y = bounds->y + (bounds->height - track_height) / 2;
    ui_progressbar_fill_rounded_rect(ctx, track_x, track_y, track_width, track_height,
                                     ui_progressbar_effective_radius(&progress->border_radius),
                                     progress->track_color);
    if (progress->determinate) {
        int indicator_width = (int)(track_width * progress->value + 0.5);
        if (indicator_width > 0) {
            if (indicator_width > track_width) {
                indicator_width = track_width;
            }
            ui_progressbar_fill_rounded_rect(ctx, track_x, track_y, indicator_width, track_height,
                                             ui_progressbar_effective_radius(&progress->border_radius),
                                             progress->indicator_color);
        }
    } else {
        ui_progressbar_update_animation(progress);
        int indicator_width = (int)(track_width * UI_PROGRESSBAR_INDETERMINATE_RATIO);
        if (indicator_width < track_height) {
            indicator_width = track_height;
        }
        if (indicator_width > track_width) {
            indicator_width = track_width;
        }
        int travel = track_width + indicator_width;
        double offset = progress->animation_phase * travel - indicator_width;
        int indicator_x = track_x + (int)offset;
        int draw_width = indicator_width;
        if (indicator_x < track_x) {
            draw_width -= track_x - indicator_x;
            indicator_x = track_x;
        }
        if (indicator_x + draw_width > track_x + track_width) {
            draw_width = track_x + track_width - indicator_x;
        }
        if (draw_width > 0) {
            ui_progressbar_fill_rounded_rect(ctx, indicator_x, track_y, draw_width, track_height,
                                             ui_progressbar_effective_radius(&progress->border_radius),
                                             progress->indicator_color);
        }
    }
    ui_rect_t track_rect = {
        .x = track_x,
        .y = track_y,
        .width = track_width,
        .height = track_height
    };
    ui_progressbar_draw_border(ctx, &track_rect, progress->border_width, progress->border_sides,
                               progress->border_color);
    return true;
}

static const ui_widget_ops_t ui_progressbar_ops = {
    .render = ui_progressbar_render,
    .handle_event = NULL,
    .destroy = NULL,
    .style_changed = ui_progressbar_apply_style
};

static void ui_progressbar_reset_animation(ui_progressbar_t *progress)
{
    if (!progress) {
        return;
    }
    progress->animation_phase = 0.0;
    clock_gettime(CLOCK_MONOTONIC, &progress->last_tick);
}

void ui_progressbar_init(ui_progressbar_t *progressbar)
{
    if (!progressbar) {
        return;
    }
    memset(progressbar, 0, sizeof(*progressbar));
    ui_widget_init(&progressbar->base, &ui_progressbar_ops);
    progressbar->value = 0.0;
    progressbar->determinate = false;
    progressbar->bar_height = 4;
    progressbar->border_radius = ui_border_radius_all(2);
    progressbar->track_color = UI_PROGRESSBAR_DEFAULT_TRACK_COLOR;
    progressbar->indicator_color = UI_PROGRESSBAR_DEFAULT_INDICATOR_COLOR;
    progressbar->border_color = 0;
    progressbar->border_width = 0;
    progressbar->border_sides = UI_BORDER_LEFT | UI_BORDER_RIGHT | UI_BORDER_TOP | UI_BORDER_BOTTOM;
    progressbar->box_shadow.enabled = false;
    progressbar->semantics_label = NULL;
    progressbar->semantics_value = NULL;
    progressbar->tooltip = NULL;
    ui_style_t default_style;
    ui_style_init(&default_style);
    default_style.background_color = progressbar->track_color;
    default_style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    default_style.foreground_color = progressbar->indicator_color;
    default_style.flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    default_style.accent_color = progressbar->indicator_color;
    default_style.flags |= UI_STYLE_FLAG_ACCENT_COLOR;
    default_style.border_color = progressbar->border_color;
    default_style.flags |= UI_STYLE_FLAG_BORDER_COLOR;
    default_style.border_width = progressbar->border_width;
    default_style.border_sides = progressbar->border_sides;
    progressbar->box_shadow.enabled = false;
    ui_widget_set_style(&progressbar->base, &default_style);
    ui_progressbar_reset_animation(progressbar);
}

ui_progressbar_t *ui_progressbar_create(void)
{
    ui_progressbar_t *progressbar = calloc(1, sizeof(*progressbar));
    if (!progressbar) {
        return NULL;
    }
    ui_progressbar_init(progressbar);
    return progressbar;
}

void ui_progressbar_destroy(ui_progressbar_t *progressbar)
{
    if (!progressbar) {
        return;
    }
    free(progressbar->semantics_label);
    free(progressbar->semantics_value);
    free(progressbar->tooltip);
    free(progressbar);
}

void ui_progressbar_set_value(ui_progressbar_t *progressbar, double value)
{
    if (!progressbar) {
        return;
    }
    if (value < 0.0) {
        value = 0.0;
    }
    if (value > 1.0) {
        value = 1.0;
    }
    progressbar->value = value;
    progressbar->determinate = true;
}

double ui_progressbar_value(const ui_progressbar_t *progressbar)
{
    if (!progressbar || !progressbar->determinate) {
        return -1.0;
    }
    return progressbar->value;
}

void ui_progressbar_clear_value(ui_progressbar_t *progressbar)
{
    if (!progressbar) {
        return;
    }
    progressbar->determinate = false;
    progressbar->value = 0.0;
    ui_progressbar_reset_animation(progressbar);
}

bool ui_progressbar_is_determinate(const ui_progressbar_t *progressbar)
{
    return progressbar ? progressbar->determinate : false;
}

void ui_progressbar_set_bar_height(ui_progressbar_t *progressbar, int height)
{
    if (!progressbar) {
        return;
    }
    progressbar->bar_height = height > 0 ? height : 1;
}

int ui_progressbar_bar_height(const ui_progressbar_t *progressbar)
{
    return progressbar ? progressbar->bar_height : 0;
}

void ui_progressbar_set_border_radius(ui_progressbar_t *progressbar,
                                      ui_border_radius_t radius)
{
    if (!progressbar) {
        return;
    }
    progressbar->border_radius = radius;
}

ui_border_radius_t ui_progressbar_border_radius(const ui_progressbar_t *progressbar)
{
    return progressbar ? progressbar->border_radius : ui_border_radius_zero();
}

void ui_progressbar_set_semantics_label(ui_progressbar_t *progressbar, const char *label)
{
    if (!progressbar) {
        return;
    }
    ui_progressbar_assign_text(&progressbar->semantics_label, label);
}

const char *ui_progressbar_semantics_label(const ui_progressbar_t *progressbar)
{
    return progressbar ? progressbar->semantics_label : NULL;
}

void ui_progressbar_set_semantics_value(ui_progressbar_t *progressbar, const char *value)
{
    if (!progressbar) {
        return;
    }
    ui_progressbar_assign_text(&progressbar->semantics_value, value);
}

const char *ui_progressbar_semantics_value(const ui_progressbar_t *progressbar)
{
    return progressbar ? progressbar->semantics_value : NULL;
}

void ui_progressbar_set_tooltip(ui_progressbar_t *progressbar, const char *tooltip)
{
    if (!progressbar) {
        return;
    }
    ui_progressbar_assign_text(&progressbar->tooltip, tooltip);
}

const char *ui_progressbar_tooltip(const ui_progressbar_t *progressbar)
{
    return progressbar ? progressbar->tooltip : NULL;
}

const ui_widget_t *ui_progressbar_widget(const ui_progressbar_t *progressbar)
{
    return progressbar ? &progressbar->base : NULL;
}

ui_widget_t *ui_progressbar_widget_mutable(ui_progressbar_t *progressbar)
{
    return progressbar ? &progressbar->base : NULL;
}
