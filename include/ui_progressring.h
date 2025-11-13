#ifndef UI_PROGRESSRING_H
#define UI_PROGRESSRING_H

#include "ui_primitives.h"
#include "ui_widget.h"

#include <stdbool.h>

typedef enum {
    UI_STROKE_CAP_BUTT,
    UI_STROKE_CAP_ROUND,
    UI_STROKE_CAP_SQUARE
} ui_stroke_cap_t;

typedef struct ui_progressring ui_progressring_t;

ui_progressring_t *ui_progressring_create(void);
void ui_progressring_destroy(ui_progressring_t *ring);
void ui_progressring_init(ui_progressring_t *ring);

const ui_widget_t *ui_progressring_widget(const ui_progressring_t *ring);
ui_widget_t *ui_progressring_widget_mutable(ui_progressring_t *ring);

void ui_progressring_set_value(ui_progressring_t *ring, double value);
double ui_progressring_value(const ui_progressring_t *ring);
void ui_progressring_clear_value(ui_progressring_t *ring);

void ui_progressring_set_track_color(ui_progressring_t *ring, ui_color_t color);
ui_color_t ui_progressring_track_color(const ui_progressring_t *ring);

void ui_progressring_set_progress_color(ui_progressring_t *ring, ui_color_t color);
ui_color_t ui_progressring_progress_color(const ui_progressring_t *ring);

void ui_progressring_set_stroke_width(ui_progressring_t *ring, double width);
double ui_progressring_stroke_width(const ui_progressring_t *ring);

void ui_progressring_set_stroke_align(ui_progressring_t *ring, double align);
double ui_progressring_stroke_align(const ui_progressring_t *ring);

void ui_progressring_set_stroke_cap(ui_progressring_t *ring, ui_stroke_cap_t cap);
ui_stroke_cap_t ui_progressring_stroke_cap(const ui_progressring_t *ring);

void ui_progressring_set_semantics_label(ui_progressring_t *ring, const char *label);
const char *ui_progressring_semantics_label(const ui_progressring_t *ring);

void ui_progressring_set_semantics_value(ui_progressring_t *ring, const char *value);
const char *ui_progressring_semantics_value(const ui_progressring_t *ring);

void ui_progressring_set_tooltip(ui_progressring_t *ring, const char *tooltip);
const char *ui_progressring_tooltip(const ui_progressring_t *ring);

#endif
