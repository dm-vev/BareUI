#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "ui_widget.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct ui_button ui_button_t;

typedef void (*ui_button_event_fn)(ui_button_t *button, void *user_data);
typedef void (*ui_button_hover_fn)(ui_button_t *button, bool hovering, void *user_data);

ui_button_t *ui_button_create(void);
void ui_button_destroy(ui_button_t *button);

void ui_button_init(ui_button_t *button);

void ui_button_set_text(ui_button_t *button, const char *text);
const char *ui_button_text(const ui_button_t *button);

void ui_button_set_font(ui_button_t *button, const bareui_font_t *font);
const bareui_font_t *ui_button_font(const ui_button_t *button);

void ui_button_set_background_color(ui_button_t *button, ui_color_t color);
ui_color_t ui_button_background_color(const ui_button_t *button);

void ui_button_set_text_color(ui_button_t *button, ui_color_t color);
ui_color_t ui_button_text_color(const ui_button_t *button);

void ui_button_set_hover_color(ui_button_t *button, ui_color_t color);
ui_color_t ui_button_hover_color(const ui_button_t *button);

void ui_button_set_pressed_color(ui_button_t *button, ui_color_t color);
ui_color_t ui_button_pressed_color(const ui_button_t *button);

void ui_button_set_rtl(ui_button_t *button, bool rtl);
bool ui_button_rtl(const ui_button_t *button);

void ui_button_set_autofocus(ui_button_t *button, bool autofocus);
bool ui_button_autofocus(const ui_button_t *button);

void ui_button_set_enabled(ui_button_t *button, bool enabled);
bool ui_button_enabled(const ui_button_t *button);

void ui_button_set_on_click(ui_button_t *button, ui_button_event_fn handler, void *user_data);
void ui_button_set_on_hover(ui_button_t *button, ui_button_hover_fn handler, void *user_data);
void ui_button_set_on_focus(ui_button_t *button, ui_button_event_fn handler, void *user_data);
void ui_button_set_on_blur(ui_button_t *button, ui_button_event_fn handler, void *user_data);
void ui_button_set_on_long_press(ui_button_t *button, ui_button_event_fn handler, void *user_data);

#endif
