#define _POSIX_C_SOURCE 200809L

#include "ui_progressring.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ui_primitives.h"

#define UI_PROGRESSRING_PI 3.14159265358979323846
#define UI_PROGRESSRING_TWO_PI (2.0 * UI_PROGRESSRING_PI)

struct ui_progressring {
    ui_widget_t base;
    ui_color_t track_color;
    ui_color_t progress_color;
    double stroke_width;
    double stroke_align;
    ui_stroke_cap_t stroke_cap;
    double value;
    bool has_value;
    char *semantics_label;
    char *semantics_value;
    char *tooltip;
};

static const ui_widget_ops_t ui_progressring_ops;

static char *ui_progressring_strdup(const char *value)
{
    if (!value) {
        return NULL;
    }
    size_t len = strlen(value) + 1;
    char *copy = malloc(len);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, value, len);
    return copy;
}

static double ui_progressring_clamp(double value, double min, double max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

static double ui_progressring_normalize_angle(double angle)
{
    double two_pi = UI_PROGRESSRING_TWO_PI;
    double normalized = fmod(angle, two_pi);
    if (normalized < 0.0) {
        normalized += two_pi;
    }
    return normalized;
}

static bool ui_progressring_angle_in_range(double angle, double start, double sweep)
{
    if (sweep <= 0.0) {
        return false;
    }
    double norm_angle = ui_progressring_normalize_angle(angle);
    double norm_start = ui_progressring_normalize_angle(start);
    double delta = norm_angle - norm_start;
    if (delta < 0.0) {
        delta += UI_PROGRESSRING_TWO_PI;
    }
    return delta <= sweep;
}

static void ui_progressring_draw_round_cap(ui_context_t *ctx, double center_x, double center_y,
                                           double radius, ui_color_t color,
                                           const ui_rect_t *bounds)
{
    if (!ctx || radius <= 0.0) {
        return;
    }
    int min_x = (int)floor(center_x - radius);
    int max_x = (int)ceil(center_x + radius);
    int min_y = (int)floor(center_y - radius);
    int max_y = (int)ceil(center_y + radius);
    if (bounds) {
        if (max_x < bounds->x || min_x > bounds->x + bounds->width) {
            return;
        }
        if (max_y < bounds->y || min_y > bounds->y + bounds->height) {
            return;
        }
        if (min_x < bounds->x) {
            min_x = bounds->x;
        }
        if (max_x > bounds->x + bounds->width) {
            max_x = bounds->x + bounds->width;
        }
        if (min_y < bounds->y) {
            min_y = bounds->y;
        }
        if (max_y > bounds->y + bounds->height) {
            max_y = bounds->y + bounds->height;
        }
    }
    double radius_sq = radius * radius;
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            double sample_x = x + 0.5;
            double sample_y = y + 0.5;
            double dx = sample_x - center_x;
            double dy = sample_y - center_y;
            if (dx * dx + dy * dy <= radius_sq) {
                ui_context_set_pixel(ctx, x, y, color);
            }
        }
    }
}

static bool ui_progressring_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    if (!ctx || !widget || !bounds) {
        return false;
    }
    ui_progressring_t *ring = (ui_progressring_t *)widget;
    if (bounds->width <= 0 || bounds->height <= 0) {
        return true;
    }

    double stroke_width = ring->stroke_width;
    if (stroke_width <= 0.0) {
        return true;
    }
    double half_stroke = stroke_width * 0.5;

    double min_dim = bounds->width < bounds->height ? bounds->width : bounds->height;
    double radius = (min_dim * 0.5) - half_stroke;
    double align = ui_progressring_clamp(ring->stroke_align, -1.0, 1.0);
    radius += align * half_stroke;
    if (radius <= 0.0) {
        return true;
    }

    double inner_radius = radius - half_stroke;
    if (inner_radius < 0.0) {
        inner_radius = 0.0;
    }
    double outer_radius = radius + half_stroke;
    double inner_sq = inner_radius * inner_radius;
    double outer_sq = outer_radius * outer_radius;

    double cx = bounds->x + bounds->width * 0.5;
    double cy = bounds->y + bounds->height * 0.5;

    double start_angle = -UI_PROGRESSRING_PI / 2.0;
    double sweep = 0.0;
    if (ring->has_value) {
        double value = ring->value;
        value = ui_progressring_clamp(value, 0.0, 1.0);
        sweep = value * UI_PROGRESSRING_TWO_PI;
    } else {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double seconds = now.tv_sec + now.tv_nsec / 1e9;
        double phase = fmod(seconds / 1.2, 1.0);
        start_angle += phase * UI_PROGRESSRING_TWO_PI;
        sweep = UI_PROGRESSRING_PI * 1.35;
    }

    if (sweep > UI_PROGRESSRING_TWO_PI) {
        sweep = UI_PROGRESSRING_TWO_PI;
    }
    if (ring->stroke_cap == UI_STROKE_CAP_SQUARE && radius > 0.0) {
        double extension = half_stroke / radius;
        start_angle -= extension;
        sweep += extension * 2.0;
        if (sweep > UI_PROGRESSRING_TWO_PI) {
            sweep = UI_PROGRESSRING_TWO_PI;
        }
    }

    double end_angle = start_angle + sweep;

    int start_y = bounds->y;
    int end_y = bounds->y + bounds->height;
    int start_x = bounds->x;
    int end_x = bounds->x + bounds->width;
    for (int y = start_y; y < end_y; ++y) {
        for (int x = start_x; x < end_x; ++x) {
            double sample_x = x + 0.5;
            double sample_y = y + 0.5;
            double dx = sample_x - cx;
            double dy = sample_y - cy;
            double dist_sq = dx * dx + dy * dy;
            if (dist_sq < inner_sq || dist_sq > outer_sq) {
                continue;
            }
            ui_context_set_pixel(ctx, x, y, ring->track_color);
            if (sweep > 0.0 && ui_progressring_angle_in_range(atan2(dy, dx), start_angle, sweep)) {
                ui_context_set_pixel(ctx, x, y, ring->progress_color);
            }
        }
    }

    if (sweep > 0.0 && ring->stroke_cap == UI_STROKE_CAP_ROUND) {
        double cap_radius = half_stroke;
        double start_caps_x = cx + cos(start_angle) * radius;
        double start_caps_y = cy + sin(start_angle) * radius;
        double end_caps_x = cx + cos(end_angle) * radius;
        double end_caps_y = cy + sin(end_angle) * radius;
        ui_progressring_draw_round_cap(ctx, start_caps_x, start_caps_y, cap_radius,
                                       ring->progress_color, bounds);
        ui_progressring_draw_round_cap(ctx, end_caps_x, end_caps_y, cap_radius,
                                       ring->progress_color, bounds);
    }

    return true;
}

static bool ui_progressring_handle_event(ui_widget_t *widget, const ui_event_t *event)
{
    (void)widget;
    (void)event;
    return false;
}

static const ui_widget_ops_t ui_progressring_ops = {
    .render = ui_progressring_render,
    .handle_event = ui_progressring_handle_event,
    .destroy = NULL,
    .style_changed = NULL,
};

void ui_progressring_init(ui_progressring_t *ring)
{
    if (!ring) {
        return;
    }
    memset(ring, 0, sizeof(*ring));
    ui_widget_init(&ring->base, &ui_progressring_ops);
    ring->track_color = ui_color_from_hex(0x1D2333);
    ring->progress_color = ui_color_from_hex(0x63E6FF);
    ring->stroke_width = 6.0;
    ring->stroke_align = 0.0;
    ring->stroke_cap = UI_STROKE_CAP_ROUND;
    ring->value = 0.0;
    ring->has_value = false;
}

ui_progressring_t *ui_progressring_create(void)
{
    ui_progressring_t *ring = calloc(1, sizeof(*ring));
    if (!ring) {
        return NULL;
    }
    ui_progressring_init(ring);
    return ring;
}

void ui_progressring_destroy(ui_progressring_t *ring)
{
    if (!ring) {
        return;
    }
    free(ring->semantics_label);
    free(ring->semantics_value);
    free(ring->tooltip);
    free(ring);
}

const ui_widget_t *ui_progressring_widget(const ui_progressring_t *ring)
{
    return ring ? &ring->base : NULL;
}

ui_widget_t *ui_progressring_widget_mutable(ui_progressring_t *ring)
{
    return ring ? &ring->base : NULL;
}

void ui_progressring_set_value(ui_progressring_t *ring, double value)
{
    if (!ring) {
        return;
    }
    ring->value = value;
    ring->has_value = true;
}

double ui_progressring_value(const ui_progressring_t *ring)
{
    return ring ? ring->value : 0.0;
}

void ui_progressring_clear_value(ui_progressring_t *ring)
{
    if (!ring) {
        return;
    }
    ring->has_value = false;
    ring->value = 0.0;
}

void ui_progressring_set_track_color(ui_progressring_t *ring, ui_color_t color)
{
    if (ring) {
        ring->track_color = color;
    }
}

ui_color_t ui_progressring_track_color(const ui_progressring_t *ring)
{
    return ring ? ring->track_color : 0;
}

void ui_progressring_set_progress_color(ui_progressring_t *ring, ui_color_t color)
{
    if (ring) {
        ring->progress_color = color;
    }
}

ui_color_t ui_progressring_progress_color(const ui_progressring_t *ring)
{
    return ring ? ring->progress_color : 0;
}

void ui_progressring_set_stroke_width(ui_progressring_t *ring, double width)
{
    if (ring && width > 0.0) {
        ring->stroke_width = width;
    }
}

double ui_progressring_stroke_width(const ui_progressring_t *ring)
{
    return ring ? ring->stroke_width : 0.0;
}

void ui_progressring_set_stroke_align(ui_progressring_t *ring, double align)
{
    if (ring) {
        ring->stroke_align = align;
    }
}

double ui_progressring_stroke_align(const ui_progressring_t *ring)
{
    return ring ? ring->stroke_align : 0.0;
}

void ui_progressring_set_stroke_cap(ui_progressring_t *ring, ui_stroke_cap_t cap)
{
    if (ring) {
        ring->stroke_cap = cap;
    }
}

ui_stroke_cap_t ui_progressring_stroke_cap(const ui_progressring_t *ring)
{
    return ring ? ring->stroke_cap : UI_STROKE_CAP_BUTT;
}

void ui_progressring_set_semantics_label(ui_progressring_t *ring, const char *label)
{
    if (!ring) {
        return;
    }
    free(ring->semantics_label);
    ring->semantics_label = ui_progressring_strdup(label);
}

const char *ui_progressring_semantics_label(const ui_progressring_t *ring)
{
    return ring ? ring->semantics_label : NULL;
}

void ui_progressring_set_semantics_value(ui_progressring_t *ring, const char *value)
{
    if (!ring) {
        return;
    }
    free(ring->semantics_value);
    ring->semantics_value = ui_progressring_strdup(value);
}

const char *ui_progressring_semantics_value(const ui_progressring_t *ring)
{
    return ring ? ring->semantics_value : NULL;
}

void ui_progressring_set_tooltip(ui_progressring_t *ring, const char *tooltip)
{
    if (!ring) {
        return;
    }
    free(ring->tooltip);
    ring->tooltip = ui_progressring_strdup(tooltip);
}

const char *ui_progressring_tooltip(const ui_progressring_t *ring)
{
    return ring ? ring->tooltip : NULL;
}
