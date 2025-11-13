#ifndef UI_COLUMN_H
#define UI_COLUMN_H

#include "ui_layout.h"
#include "ui_widget.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    UI_SCROLL_MODE_NONE,
    UI_SCROLL_MODE_ENABLED
} ui_scroll_mode_t;

typedef struct ui_column ui_column_t;

ui_column_t *ui_column_create(void);
void ui_column_destroy(ui_column_t *column);

void ui_column_init(ui_column_t *column);

bool ui_column_add_control(ui_column_t *column, ui_widget_t *control, bool expand,
                           const char *key);
bool ui_column_remove_control(ui_column_t *column, ui_widget_t *control);
size_t ui_column_control_count(const ui_column_t *column);

void ui_column_set_alignment(ui_column_t *column, ui_main_axis_alignment_t alignment);
ui_main_axis_alignment_t ui_column_alignment(const ui_column_t *column);

void ui_column_set_horizontal_alignment(ui_column_t *column,
                                        ui_cross_axis_alignment_t alignment);
ui_cross_axis_alignment_t ui_column_horizontal_alignment(const ui_column_t *column);

void ui_column_set_spacing(ui_column_t *column, int spacing);
int ui_column_spacing(const ui_column_t *column);

void ui_column_set_auto_scroll(ui_column_t *column, bool auto_scroll);
bool ui_column_auto_scroll(const ui_column_t *column);

void ui_column_set_scroll_mode(ui_column_t *column, ui_scroll_mode_t mode);
ui_scroll_mode_t ui_column_scroll_mode(const ui_column_t *column);

void ui_column_set_scroll_offset(ui_column_t *column, int offset);
int ui_column_scroll_offset(const ui_column_t *column);

void ui_column_set_wrap(ui_column_t *column, bool wrap);
bool ui_column_wrap(const ui_column_t *column);

void ui_column_set_run_alignment(ui_column_t *column, ui_main_axis_alignment_t alignment);
ui_main_axis_alignment_t ui_column_run_alignment(const ui_column_t *column);

void ui_column_set_run_spacing(ui_column_t *column, int spacing);
int ui_column_run_spacing(const ui_column_t *column);

void ui_column_set_wrapping_spacing(ui_column_t *column, int spacing);

void ui_column_set_tight(ui_column_t *column, bool tight);
bool ui_column_tight(const ui_column_t *column);

void ui_column_set_rtl(ui_column_t *column, bool rtl);
bool ui_column_rtl(const ui_column_t *column);

void ui_column_set_on_scroll_interval(ui_column_t *column, int interval_ms);
int ui_column_on_scroll_interval(const ui_column_t *column);

void ui_column_scroll_to(ui_column_t *column, int offset, int delta, const char *key,
                         int duration_ms, double curve);

const ui_widget_t *ui_column_widget(const ui_column_t *column);
ui_widget_t *ui_column_widget_mutable(ui_column_t *column);

#endif
