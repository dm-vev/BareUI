#ifndef UI_CHECKBOX_H
#define UI_CHECKBOX_H

#include "ui_font.h"
#include "ui_widget.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_CHECKBOX_STATE_UNCHECKED = 0,
    UI_CHECKBOX_STATE_CHECKED,
    UI_CHECKBOX_STATE_INDETERMINATE
} ui_checkbox_state_t;

typedef enum {
    UI_CHECKBOX_LABEL_POSITION_RIGHT = 0,
    UI_CHECKBOX_LABEL_POSITION_LEFT
} ui_checkbox_label_position_t;

typedef struct ui_checkbox ui_checkbox_t;

typedef void (*ui_checkbox_change_fn)(ui_checkbox_t *checkbox, ui_checkbox_state_t state,
                                      void *user_data);
typedef void (*ui_checkbox_event_fn)(ui_checkbox_t *checkbox, void *user_data);

ui_checkbox_t *ui_checkbox_create(void);
void ui_checkbox_destroy(ui_checkbox_t *checkbox);

void ui_checkbox_init(ui_checkbox_t *checkbox);

void ui_checkbox_set_state(ui_checkbox_t *checkbox, ui_checkbox_state_t state);
ui_checkbox_state_t ui_checkbox_state(const ui_checkbox_t *checkbox);

void ui_checkbox_set_tristate(ui_checkbox_t *checkbox, bool tristate);
bool ui_checkbox_tristate(const ui_checkbox_t *checkbox);

void ui_checkbox_set_active_color(ui_checkbox_t *checkbox, ui_color_t color);
ui_color_t ui_checkbox_active_color(const ui_checkbox_t *checkbox);

void ui_checkbox_set_fill_color(ui_checkbox_t *checkbox, ui_color_t color);
ui_color_t ui_checkbox_fill_color(const ui_checkbox_t *checkbox);

void ui_checkbox_set_border_color(ui_checkbox_t *checkbox, ui_color_t color);
ui_color_t ui_checkbox_border_color(const ui_checkbox_t *checkbox);

void ui_checkbox_set_check_color(ui_checkbox_t *checkbox, ui_color_t color);
ui_color_t ui_checkbox_check_color(const ui_checkbox_t *checkbox);

void ui_checkbox_set_hover_color(ui_checkbox_t *checkbox, ui_color_t color);
ui_color_t ui_checkbox_hover_color(const ui_checkbox_t *checkbox);

void ui_checkbox_set_label(ui_checkbox_t *checkbox, const char *label);
const char *ui_checkbox_label(const ui_checkbox_t *checkbox);

void ui_checkbox_set_label_position(ui_checkbox_t *checkbox,
                                    ui_checkbox_label_position_t position);
ui_checkbox_label_position_t ui_checkbox_label_position(const ui_checkbox_t *checkbox);

void ui_checkbox_set_font(ui_checkbox_t *checkbox, const bareui_font_t *font);
const bareui_font_t *ui_checkbox_font(const ui_checkbox_t *checkbox);

void ui_checkbox_set_enabled(ui_checkbox_t *checkbox, bool enabled);
bool ui_checkbox_enabled(const ui_checkbox_t *checkbox);

void ui_checkbox_set_autofocus(ui_checkbox_t *checkbox, bool autofocus);
bool ui_checkbox_autofocus(const ui_checkbox_t *checkbox);

void ui_checkbox_set_is_error(ui_checkbox_t *checkbox, bool is_error);
bool ui_checkbox_is_error(const ui_checkbox_t *checkbox);

void ui_checkbox_set_on_change(ui_checkbox_t *checkbox, ui_checkbox_change_fn handler,
                               void *user_data);
void ui_checkbox_set_on_focus(ui_checkbox_t *checkbox, ui_checkbox_event_fn handler,
                              void *user_data);
void ui_checkbox_set_on_blur(ui_checkbox_t *checkbox, ui_checkbox_event_fn handler,
                             void *user_data);

const ui_widget_t *ui_checkbox_widget(const ui_checkbox_t *checkbox);
ui_widget_t *ui_checkbox_widget_mutable(ui_checkbox_t *checkbox);

#ifdef __cplusplus
}
#endif

#endif
