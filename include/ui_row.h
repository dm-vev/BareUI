#ifndef UI_ROW_H
#define UI_ROW_H

#include "ui_layout.h"
#include "ui_widget.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    UI_SCROLL_DIRECTION_NONE,
    UI_SCROLL_DIRECTION_HORIZONTAL
} ui_row_scroll_mode_t;

typedef struct ui_row ui_row_t;

ui_row_t *ui_row_create(void);
void ui_row_destroy(ui_row_t *row);

void ui_row_init(ui_row_t *row);

bool ui_row_add_control(ui_row_t *row, ui_widget_t *control, bool expand, const char *key);
bool ui_row_remove_control(ui_row_t *row, ui_widget_t *control);
size_t ui_row_control_count(const ui_row_t *row);

void ui_row_set_alignment(ui_row_t *row, ui_main_axis_alignment_t alignment);
ui_main_axis_alignment_t ui_row_alignment(const ui_row_t *row);

void ui_row_set_vertical_alignment(ui_row_t *row, ui_cross_axis_alignment_t alignment);
ui_cross_axis_alignment_t ui_row_vertical_alignment(const ui_row_t *row);

void ui_row_set_spacing(ui_row_t *row, int spacing);
int ui_row_spacing(const ui_row_t *row);

void ui_row_set_auto_scroll(ui_row_t *row, bool auto_scroll);
bool ui_row_auto_scroll(const ui_row_t *row);

void ui_row_set_scroll_mode(ui_row_t *row, ui_row_scroll_mode_t mode);
ui_row_scroll_mode_t ui_row_scroll_mode(const ui_row_t *row);

void ui_row_set_scroll_offset(ui_row_t *row, int offset);
int ui_row_scroll_offset(const ui_row_t *row);

void ui_row_set_wrap(ui_row_t *row, bool wrap);
bool ui_row_wrap(const ui_row_t *row);

void ui_row_set_run_alignment(ui_row_t *row, ui_main_axis_alignment_t alignment);
ui_main_axis_alignment_t ui_row_run_alignment(const ui_row_t *row);

void ui_row_set_run_spacing(ui_row_t *row, int spacing);
int ui_row_run_spacing(const ui_row_t *row);

void ui_row_set_tight(ui_row_t *row, bool tight);
bool ui_row_tight(const ui_row_t *row);

void ui_row_set_rtl(ui_row_t *row, bool rtl);
bool ui_row_rtl(const ui_row_t *row);

void ui_row_set_on_scroll_interval(ui_row_t *row, int interval_ms);
int ui_row_on_scroll_interval(const ui_row_t *row);

void ui_row_scroll_to(ui_row_t *row, int offset, int delta, const char *key,
                      int duration_ms, double curve);

const ui_widget_t *ui_row_widget(const ui_row_t *row);
ui_widget_t *ui_row_widget_mutable(ui_row_t *row);

#endif
