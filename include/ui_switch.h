#ifndef UI_SWITCH_H
#define UI_SWITCH_H

#include "ui_font.h"
#include "ui_widget.h"

#include <stdbool.h>

typedef enum {
    UI_SWITCH_LABEL_POSITION_RIGHT,
    UI_SWITCH_LABEL_POSITION_LEFT
} ui_switch_label_position_t;

typedef struct ui_switch ui_switch_t;

typedef void (*ui_switch_change_fn)(ui_switch_t *sw, bool value, void *user_data);
typedef void (*ui_switch_event_fn)(ui_switch_t *sw, void *user_data);

ui_switch_t *ui_switch_create(void);
void ui_switch_destroy(ui_switch_t *sw);

void ui_switch_init(ui_switch_t *sw);

void ui_switch_set_value(ui_switch_t *sw, bool value);
bool ui_switch_value(const ui_switch_t *sw);

void ui_switch_set_enabled(ui_switch_t *sw, bool enabled);
bool ui_switch_enabled(const ui_switch_t *sw);

void ui_switch_set_active_color(ui_switch_t *sw, ui_color_t color);
ui_color_t ui_switch_active_color(const ui_switch_t *sw);

void ui_switch_set_active_track_color(ui_switch_t *sw, ui_color_t color);
ui_color_t ui_switch_active_track_color(const ui_switch_t *sw);

void ui_switch_set_inactive_track_color(ui_switch_t *sw, ui_color_t color);
ui_color_t ui_switch_inactive_track_color(const ui_switch_t *sw);

void ui_switch_set_inactive_thumb_color(ui_switch_t *sw, ui_color_t color);
ui_color_t ui_switch_inactive_thumb_color(const ui_switch_t *sw);

void ui_switch_set_hover_color(ui_switch_t *sw, ui_color_t color);
ui_color_t ui_switch_hover_color(const ui_switch_t *sw);

void ui_switch_set_focus_color(ui_switch_t *sw, ui_color_t color);
ui_color_t ui_switch_focus_color(const ui_switch_t *sw);

void ui_switch_set_track_outline_color(ui_switch_t *sw, ui_color_t color);
ui_color_t ui_switch_track_outline_color(const ui_switch_t *sw);

void ui_switch_set_track_outline_width(ui_switch_t *sw, int width);
int ui_switch_track_outline_width(const ui_switch_t *sw);

void ui_switch_set_label(ui_switch_t *sw, const char *label);
const char *ui_switch_label(const ui_switch_t *sw);

void ui_switch_set_label_position(ui_switch_t *sw, ui_switch_label_position_t position);
ui_switch_label_position_t ui_switch_label_position(const ui_switch_t *sw);

void ui_switch_set_font(ui_switch_t *sw, const bareui_font_t *font);
const bareui_font_t *ui_switch_font(const ui_switch_t *sw);

void ui_switch_set_on_change(ui_switch_t *sw, ui_switch_change_fn handler, void *user_data);
void ui_switch_set_on_focus(ui_switch_t *sw, ui_switch_event_fn handler, void *user_data);
void ui_switch_set_on_blur(ui_switch_t *sw, ui_switch_event_fn handler, void *user_data);

const ui_widget_t *ui_switch_widget(const ui_switch_t *sw);
ui_widget_t *ui_switch_widget_mutable(ui_switch_t *sw);

#endif
