#ifndef UI_APPBAR_H
#define UI_APPBAR_H

#include "ui_widget.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_APPBAR_CLIP_BEHAVIOR_NONE,
    UI_APPBAR_CLIP_BEHAVIOR_HARD_EDGE,
    UI_APPBAR_CLIP_BEHAVIOR_ANTI_ALIAS
} ui_appbar_clip_behavior_t;

typedef struct ui_appbar ui_appbar_t;

ui_appbar_t *ui_appbar_create(void);
void ui_appbar_destroy(ui_appbar_t *appbar);
void ui_appbar_init(ui_appbar_t *appbar);

const ui_widget_t *ui_appbar_widget(const ui_appbar_t *appbar);
ui_widget_t *ui_appbar_widget_mutable(ui_appbar_t *appbar);

bool ui_appbar_set_leading(ui_appbar_t *appbar, ui_widget_t *leading);
ui_widget_t *ui_appbar_leading(const ui_appbar_t *appbar);

bool ui_appbar_set_title(ui_appbar_t *appbar, ui_widget_t *title);
ui_widget_t *ui_appbar_title(const ui_appbar_t *appbar);

bool ui_appbar_add_action(ui_appbar_t *appbar, ui_widget_t *action, const char *key);
bool ui_appbar_remove_action(ui_appbar_t *appbar, ui_widget_t *action);
bool ui_appbar_remove_action_by_key(ui_appbar_t *appbar, const char *key);
size_t ui_appbar_action_count(const ui_appbar_t *appbar);

void ui_appbar_clear_actions(ui_appbar_t *appbar);

void ui_appbar_set_adaptive(ui_appbar_t *appbar, bool adaptive);
bool ui_appbar_adaptive(const ui_appbar_t *appbar);

void ui_appbar_set_automatically_imply_leading(ui_appbar_t *appbar, bool imply);
bool ui_appbar_automatically_imply_leading(const ui_appbar_t *appbar);

void ui_appbar_set_center_title(ui_appbar_t *appbar, bool center);
bool ui_appbar_center_title(const ui_appbar_t *appbar);

void ui_appbar_set_title_spacing(ui_appbar_t *appbar, int spacing);
int ui_appbar_title_spacing(const ui_appbar_t *appbar);

void ui_appbar_set_leading_width(ui_appbar_t *appbar, int width);
int ui_appbar_leading_width(const ui_appbar_t *appbar);

void ui_appbar_set_toolbar_height(ui_appbar_t *appbar, int height);
int ui_appbar_toolbar_height(const ui_appbar_t *appbar);

void ui_appbar_set_toolbar_opacity(ui_appbar_t *appbar, double opacity);
double ui_appbar_toolbar_opacity(const ui_appbar_t *appbar);

void ui_appbar_set_background_color(ui_appbar_t *appbar, ui_color_t color);
ui_color_t ui_appbar_background_color(const ui_appbar_t *appbar);

void ui_appbar_set_content_color(ui_appbar_t *appbar, ui_color_t color);
ui_color_t ui_appbar_content_color(const ui_appbar_t *appbar);

void ui_appbar_set_force_material_transparency(ui_appbar_t *appbar, bool force);
bool ui_appbar_force_material_transparency(const ui_appbar_t *appbar);

void ui_appbar_set_elevation(ui_appbar_t *appbar, double elevation);
double ui_appbar_elevation(const ui_appbar_t *appbar);

void ui_appbar_set_elevation_on_scroll(ui_appbar_t *appbar, double elevation);
double ui_appbar_elevation_on_scroll(const ui_appbar_t *appbar);

void ui_appbar_set_shadow_color(ui_appbar_t *appbar, ui_color_t color);
ui_color_t ui_appbar_shadow_color(const ui_appbar_t *appbar);

void ui_appbar_set_surface_tint_color(ui_appbar_t *appbar, ui_color_t color);
ui_color_t ui_appbar_surface_tint_color(const ui_appbar_t *appbar);

void ui_appbar_set_clip_behavior(ui_appbar_t *appbar, ui_appbar_clip_behavior_t behavior);
ui_appbar_clip_behavior_t ui_appbar_clip_behavior(const ui_appbar_t *appbar);

void ui_appbar_set_exclude_header_semantics(ui_appbar_t *appbar, bool exclude);
bool ui_appbar_exclude_header_semantics(const ui_appbar_t *appbar);

void ui_appbar_set_is_secondary(ui_appbar_t *appbar, bool secondary);
bool ui_appbar_is_secondary(const ui_appbar_t *appbar);

#ifdef __cplusplus
}
#endif

#endif
