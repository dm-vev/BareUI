#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include "ui_widget.h"

#include <stdbool.h>

typedef enum {
    UI_SLIDER_INTERACTION_TAP_AND_SLIDE,
    UI_SLIDER_INTERACTION_DRAG_ONLY
} ui_slider_interaction_t;

typedef enum {
    UI_MOUSE_CURSOR_DEFAULT,
    UI_MOUSE_CURSOR_POINTER,
    UI_MOUSE_CURSOR_GRAB,
    UI_MOUSE_CURSOR_MOVE
} ui_mouse_cursor_t;

typedef struct ui_slider ui_slider_t;

typedef void (*ui_slider_event_fn)(ui_slider_t *slider, void *user_data);
typedef void (*ui_slider_value_event_fn)(ui_slider_t *slider, double value, void *user_data);

ui_slider_t *ui_slider_create(void);
void ui_slider_destroy(ui_slider_t *slider);

void ui_slider_init(ui_slider_t *slider);

void ui_slider_set_value(ui_slider_t *slider, double value);
double ui_slider_value(const ui_slider_t *slider);

void ui_slider_set_min(ui_slider_t *slider, double min);
double ui_slider_min(const ui_slider_t *slider);

void ui_slider_set_max(ui_slider_t *slider, double max);
double ui_slider_max(const ui_slider_t *slider);

void ui_slider_set_divisions(ui_slider_t *slider, int divisions);
int ui_slider_divisions(const ui_slider_t *slider);

void ui_slider_set_active_color(ui_slider_t *slider, ui_color_t color);
ui_color_t ui_slider_active_color(const ui_slider_t *slider);

void ui_slider_set_inactive_color(ui_slider_t *slider, ui_color_t color);
ui_color_t ui_slider_inactive_color(const ui_slider_t *slider);

void ui_slider_set_thumb_color(ui_slider_t *slider, ui_color_t color);
ui_color_t ui_slider_thumb_color(const ui_slider_t *slider);

void ui_slider_set_overlay_color(ui_slider_t *slider, ui_color_t color);
ui_color_t ui_slider_overlay_color(const ui_slider_t *slider);

void ui_slider_set_secondary_active_color(ui_slider_t *slider, ui_color_t color);
ui_color_t ui_slider_secondary_active_color(const ui_slider_t *slider);

void ui_slider_set_secondary_track_value(ui_slider_t *slider, double value);
double ui_slider_secondary_track_value(const ui_slider_t *slider);
bool ui_slider_has_secondary_track_value(const ui_slider_t *slider);
void ui_slider_clear_secondary_track_value(ui_slider_t *slider);

void ui_slider_set_label(ui_slider_t *slider, const char *label);
const char *ui_slider_label(const ui_slider_t *slider);

void ui_slider_set_round(ui_slider_t *slider, int decimals);
int ui_slider_round(const ui_slider_t *slider);

void ui_slider_set_interaction(ui_slider_t *slider, ui_slider_interaction_t interaction);
ui_slider_interaction_t ui_slider_interaction(const ui_slider_t *slider);

void ui_slider_set_autofocus(ui_slider_t *slider, bool autofocus);
bool ui_slider_autofocus(const ui_slider_t *slider);

void ui_slider_set_adaptive(ui_slider_t *slider, bool adaptive);
bool ui_slider_adaptive(const ui_slider_t *slider);

void ui_slider_set_mouse_cursor(ui_slider_t *slider, ui_mouse_cursor_t cursor);
ui_mouse_cursor_t ui_slider_mouse_cursor(const ui_slider_t *slider);

void ui_slider_set_on_change(ui_slider_t *slider, ui_slider_value_event_fn handler,
                             void *user_data);
void ui_slider_set_on_change_start(ui_slider_t *slider, ui_slider_value_event_fn handler,
                                   void *user_data);
void ui_slider_set_on_change_end(ui_slider_t *slider, ui_slider_value_event_fn handler,
                                 void *user_data);

void ui_slider_set_on_focus(ui_slider_t *slider, ui_slider_event_fn handler, void *user_data);
void ui_slider_set_on_blur(ui_slider_t *slider, ui_slider_event_fn handler, void *user_data);

const ui_widget_t *ui_slider_widget(const ui_slider_t *slider);
ui_widget_t *ui_slider_widget_mutable(ui_slider_t *slider);

#endif
