#ifndef UI_RADIO_H
#define UI_RADIO_H

#include "ui_font.h"
#include "ui_mouse_cursor.h"
#include "ui_widget.h"

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_RADIO_VALUE_NONE INT32_MIN

typedef enum {
    UI_RADIO_LABEL_POSITION_RIGHT = 0,
    UI_RADIO_LABEL_POSITION_LEFT
} ui_radio_label_position_t;

typedef enum {
    UI_RADIO_VISUAL_DENSITY_COMFORTABLE = 0,
    UI_RADIO_VISUAL_DENSITY_COMPACT
} ui_radio_visual_density_t;

typedef struct ui_radio ui_radio_t;
typedef struct ui_radio_group ui_radio_group_t;

typedef void (*ui_radio_change_fn)(ui_radio_t *radio, int32_t value, void *user_data);
typedef void (*ui_radio_event_fn)(ui_radio_t *radio, void *user_data);
typedef void (*ui_radio_group_change_fn)(ui_radio_group_t *group, int32_t value,
                                          void *user_data);

ui_radio_t *ui_radio_create(void);
void ui_radio_destroy(ui_radio_t *radio);

void ui_radio_init(ui_radio_t *radio);

void ui_radio_set_value(ui_radio_t *radio, int32_t value);
int32_t ui_radio_value(const ui_radio_t *radio);

void ui_radio_set_selected(ui_radio_t *radio, bool selected);
bool ui_radio_selected(const ui_radio_t *radio);

void ui_radio_set_group(ui_radio_t *radio, ui_radio_group_t *group);
ui_radio_group_t *ui_radio_group(const ui_radio_t *radio);

void ui_radio_set_enabled(ui_radio_t *radio, bool enabled);
bool ui_radio_enabled(const ui_radio_t *radio);

void ui_radio_set_toggleable(ui_radio_t *radio, bool toggleable);
bool ui_radio_toggleable(const ui_radio_t *radio);

void ui_radio_set_autofocus(ui_radio_t *radio, bool autofocus);
bool ui_radio_autofocus(const ui_radio_t *radio);

void ui_radio_set_adaptive(ui_radio_t *radio, bool adaptive);
bool ui_radio_adaptive(const ui_radio_t *radio);

void ui_radio_set_active_color(ui_radio_t *radio, ui_color_t color);
ui_color_t ui_radio_active_color(const ui_radio_t *radio);

void ui_radio_set_fill_color(ui_radio_t *radio, ui_color_t color);
ui_color_t ui_radio_fill_color(const ui_radio_t *radio);

void ui_radio_set_border_color(ui_radio_t *radio, ui_color_t color);
ui_color_t ui_radio_border_color(const ui_radio_t *radio);

void ui_radio_set_hover_color(ui_radio_t *radio, ui_color_t color);
ui_color_t ui_radio_hover_color(const ui_radio_t *radio);

void ui_radio_set_focus_color(ui_radio_t *radio, ui_color_t color);
ui_color_t ui_radio_focus_color(const ui_radio_t *radio);

void ui_radio_set_overlay_color(ui_radio_t *radio, ui_color_t color);
ui_color_t ui_radio_overlay_color(const ui_radio_t *radio);

void ui_radio_set_splash_radius(ui_radio_t *radio, int radius);
int ui_radio_splash_radius(const ui_radio_t *radio);

void ui_radio_set_label(ui_radio_t *radio, const char *label);
const char *ui_radio_label(const ui_radio_t *radio);

void ui_radio_set_label_font(ui_radio_t *radio, const bareui_font_t *font);
const bareui_font_t *ui_radio_label_font(const ui_radio_t *radio);

void ui_radio_set_label_color(ui_radio_t *radio, ui_color_t color);
ui_color_t ui_radio_label_color(const ui_radio_t *radio);

void ui_radio_set_label_position(ui_radio_t *radio, ui_radio_label_position_t position);
ui_radio_label_position_t ui_radio_label_position(const ui_radio_t *radio);

void ui_radio_set_visual_density(ui_radio_t *radio, ui_radio_visual_density_t density);
ui_radio_visual_density_t ui_radio_visual_density(const ui_radio_t *radio);

void ui_radio_set_mouse_cursor(ui_radio_t *radio, ui_mouse_cursor_t cursor);
ui_mouse_cursor_t ui_radio_mouse_cursor(const ui_radio_t *radio);

void ui_radio_set_on_change(ui_radio_t *radio, ui_radio_change_fn handler, void *user_data);
void ui_radio_set_on_focus(ui_radio_t *radio, ui_radio_event_fn handler, void *user_data);
void ui_radio_set_on_blur(ui_radio_t *radio, ui_radio_event_fn handler, void *user_data);

const ui_widget_t *ui_radio_widget(const ui_radio_t *radio);
ui_widget_t *ui_radio_widget_mutable(ui_radio_t *radio);

ui_radio_group_t *ui_radio_group_create(void);
void ui_radio_group_destroy(ui_radio_group_t *group);

void ui_radio_group_set_value(ui_radio_group_t *group, int32_t value);
int32_t ui_radio_group_value(const ui_radio_group_t *group);

void ui_radio_group_set_on_change(ui_radio_group_t *group, ui_radio_group_change_fn handler,
                                  void *user_data);

#ifdef __cplusplus
}
#endif

#endif
