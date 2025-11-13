#ifndef UI_TEXT_H
#define UI_TEXT_H

#include "ui_layout.h"
#include "ui_widget.h"

#include <stdbool.h>

typedef enum {
    UI_TEXT_ALIGN_LEFT,
    UI_TEXT_ALIGN_CENTER,
    UI_TEXT_ALIGN_RIGHT
} ui_text_align_t;

typedef enum {
    UI_TEXT_OVERFLOW_CLIP,
    UI_TEXT_OVERFLOW_FADE
} ui_text_overflow_t;

typedef struct ui_text ui_text_t;

ui_text_t *ui_text_create(void);
void ui_text_destroy(ui_text_t *text);

void ui_text_init(ui_text_t *text);

void ui_text_set_value(ui_text_t *text, const char *value);
const char *ui_text_value(const ui_text_t *text);

void ui_text_set_color(ui_text_t *text, ui_color_t color);
void ui_text_set_background_color(ui_text_t *text, ui_color_t color);

void ui_text_set_font(ui_text_t *text, const bareui_font_t *font);
const bareui_font_t *ui_text_font(const ui_text_t *text);

void ui_text_set_align(ui_text_t *text, ui_text_align_t align);
ui_text_align_t ui_text_align(const ui_text_t *text);

void ui_text_set_overflow(ui_text_t *text, ui_text_overflow_t overflow);
ui_text_overflow_t ui_text_overflow(const ui_text_t *text);

void ui_text_set_no_wrap(ui_text_t *text, bool no_wrap);
bool ui_text_no_wrap(const ui_text_t *text);

void ui_text_set_max_lines(ui_text_t *text, int max_lines);
int ui_text_max_lines(const ui_text_t *text);

void ui_text_set_line_spacing(ui_text_t *text, int spacing);
int ui_text_line_spacing(const ui_text_t *text);

void ui_text_set_rtl(ui_text_t *text, bool rtl);
bool ui_text_rtl(const ui_text_t *text);

void ui_text_set_italic(ui_text_t *text, bool italic);
bool ui_text_italic(const ui_text_t *text);

const ui_widget_t *ui_text_widget(const ui_text_t *text);
ui_widget_t *ui_text_widget_mutable(ui_text_t *text);

#endif
