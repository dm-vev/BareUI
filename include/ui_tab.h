#ifndef UI_TAB_H
#define UI_TAB_H

#include "ui_mouse_cursor.h"
#include "ui_primitives.h"
#include "ui_widget.h"
#include "ui_border_radius.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int top;
    int bottom;
    int left;
    int right;
} ui_padding_t;

typedef enum {
    UI_TAB_CLIP_BEHAVIOR_NONE,
    UI_TAB_CLIP_BEHAVIOR_HARD_EDGE,
    UI_TAB_CLIP_BEHAVIOR_ANTI_ALIAS
} ui_tab_clip_behavior_t;

typedef enum {
    UI_TAB_ALIGNMENT_FILL,
    UI_TAB_ALIGNMENT_START,
    UI_TAB_ALIGNMENT_CENTER,
    UI_TAB_ALIGNMENT_END
} ui_tab_alignment_t;

typedef struct ui_tab ui_tab_t;
typedef struct ui_tabs ui_tabs_t;

typedef void (*ui_tabs_change_fn)(ui_tabs_t *tabs, size_t selected_index, void *user_data);
typedef void (*ui_tabs_click_fn)(ui_tabs_t *tabs, size_t index, void *user_data);

ui_tab_t *ui_tab_create(void);
void ui_tab_destroy(ui_tab_t *tab);

void ui_tab_set_text(ui_tab_t *tab, const char *text);
const char *ui_tab_text(const ui_tab_t *tab);

void ui_tab_set_icon(ui_tab_t *tab, ui_widget_t *icon);
ui_widget_t *ui_tab_icon(const ui_tab_t *tab);

void ui_tab_set_tab_content(ui_tab_t *tab, ui_widget_t *tab_content);
ui_widget_t *ui_tab_tab_content(const ui_tab_t *tab);

void ui_tab_set_content(ui_tab_t *tab, ui_widget_t *content);
ui_widget_t *ui_tab_content(const ui_tab_t *tab);

ui_tabs_t *ui_tabs_create(void);
void ui_tabs_destroy(ui_tabs_t *tabs);
void ui_tabs_init(ui_tabs_t *tabs);

const ui_widget_t *ui_tabs_widget(const ui_tabs_t *tabs);
ui_widget_t *ui_tabs_widget_mutable(ui_tabs_t *tabs);

size_t ui_tabs_tab_count(const ui_tabs_t *tabs);

bool ui_tabs_add_tab(ui_tabs_t *tabs, ui_tab_t *tab);
bool ui_tabs_remove_tab(ui_tabs_t *tabs, ui_tab_t *tab);
void ui_tabs_clear(ui_tabs_t *tabs);

void ui_tabs_set_selected_index(ui_tabs_t *tabs, size_t index);
size_t ui_tabs_selected_index(const ui_tabs_t *tabs);

void ui_tabs_set_clip_behavior(ui_tabs_t *tabs, ui_tab_clip_behavior_t behavior);
ui_tab_clip_behavior_t ui_tabs_clip_behavior(const ui_tabs_t *tabs);

void ui_tabs_set_scrollable(ui_tabs_t *tabs, bool scrollable);
bool ui_tabs_scrollable(const ui_tabs_t *tabs);

void ui_tabs_set_tab_alignment(ui_tabs_t *tabs, ui_tab_alignment_t alignment);
ui_tab_alignment_t ui_tabs_tab_alignment(const ui_tabs_t *tabs);

void ui_tabs_set_padding(ui_tabs_t *tabs, ui_padding_t padding);
ui_padding_t ui_tabs_padding(const ui_tabs_t *tabs);

void ui_tabs_set_label_padding(ui_tabs_t *tabs, ui_padding_t padding);
ui_padding_t ui_tabs_label_padding(const ui_tabs_t *tabs);

void ui_tabs_set_indicator_color(ui_tabs_t *tabs, ui_color_t color);
ui_color_t ui_tabs_indicator_color(const ui_tabs_t *tabs);

void ui_tabs_set_indicator_thickness(ui_tabs_t *tabs, int thickness);
int ui_tabs_indicator_thickness(const ui_tabs_t *tabs);

void ui_tabs_set_indicator_padding(ui_tabs_t *tabs, int padding);
int ui_tabs_indicator_padding(const ui_tabs_t *tabs);

void ui_tabs_set_indicator_tab_size(ui_tabs_t *tabs, bool full_tab);
bool ui_tabs_indicator_tab_size(const ui_tabs_t *tabs);

void ui_tabs_set_indicator_border_radius(ui_tabs_t *tabs, ui_border_radius_t radius);
ui_border_radius_t ui_tabs_indicator_border_radius(const ui_tabs_t *tabs);

void ui_tabs_set_indicator_border_side(ui_tabs_t *tabs, ui_border_side_t sides);
ui_border_side_t ui_tabs_indicator_border_side(const ui_tabs_t *tabs);

void ui_tabs_set_divider_color(ui_tabs_t *tabs, ui_color_t color);
ui_color_t ui_tabs_divider_color(const ui_tabs_t *tabs);

void ui_tabs_set_divider_height(ui_tabs_t *tabs, int height);
int ui_tabs_divider_height(const ui_tabs_t *tabs);

void ui_tabs_set_label_color(ui_tabs_t *tabs, ui_color_t color);
ui_color_t ui_tabs_label_color(const ui_tabs_t *tabs);

void ui_tabs_set_unselected_label_color(ui_tabs_t *tabs, ui_color_t color);
ui_color_t ui_tabs_unselected_label_color(const ui_tabs_t *tabs);

void ui_tabs_set_overlay_color(ui_tabs_t *tabs, ui_color_t color);
ui_color_t ui_tabs_overlay_color(const ui_tabs_t *tabs);

void ui_tabs_set_enable_feedback(ui_tabs_t *tabs, bool enabled);
bool ui_tabs_enable_feedback(const ui_tabs_t *tabs);

void ui_tabs_set_splash_border_radius(ui_tabs_t *tabs, ui_border_radius_t radius);
ui_border_radius_t ui_tabs_splash_border_radius(const ui_tabs_t *tabs);

void ui_tabs_set_mouse_cursor(ui_tabs_t *tabs, ui_mouse_cursor_t cursor);
ui_mouse_cursor_t ui_tabs_mouse_cursor(const ui_tabs_t *tabs);

void ui_tabs_set_on_change(ui_tabs_t *tabs, ui_tabs_change_fn handler, void *user_data);
void ui_tabs_set_on_click(ui_tabs_t *tabs, ui_tabs_click_fn handler, void *user_data);

#endif
