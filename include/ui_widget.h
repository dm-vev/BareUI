#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include "ui_primitives.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    int x;
    int y;
    int width;
    int height;
} ui_rect_t;

typedef struct ui_widget ui_widget_t;

typedef struct {
    bool (*render)(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds);
    bool (*handle_event)(ui_widget_t *widget, const ui_event_t *event);
    void (*destroy)(ui_widget_t *widget);
} ui_widget_ops_t;

typedef struct {
    const char *name;
    uint32_t value;
} ui_style_prop_t;

#define UI_STYLE_MAX_CUSTOM_PROPS 16

typedef struct {
    ui_color_t background_color;
    ui_color_t foreground_color;
    ui_color_t border_color;
    ui_color_t accent_color;
    int border_width;
    int border_radius;
    int padding_left;
    int padding_right;
    int padding_top;
    int padding_bottom;
    int margin_left;
    int margin_right;
    int margin_top;
    int margin_bottom;
    bool shadow_enabled;
    int shadow_offset_x;
    int shadow_offset_y;
    ui_color_t shadow_color;
    const void *extra;
    size_t extra_size;
    ui_style_prop_t custom_props[UI_STYLE_MAX_CUSTOM_PROPS];
    size_t custom_count;
} ui_style_t;

void ui_style_init(ui_style_t *style);
void ui_style_copy(ui_style_t *dst, const ui_style_t *src);
bool ui_style_set_custom_prop(ui_style_t *style, const char *key, uint32_t value);
bool ui_style_get_custom_prop(const ui_style_t *style, const char *key, uint32_t *out);

struct ui_widget {
    ui_widget_t *parent;
    ui_widget_t *first_child;
    ui_widget_t *next_sibling;
    const ui_widget_ops_t *ops;
    ui_rect_t bounds;
    void *user_data;
    bool visible;
    ui_style_t style;
};

void ui_widget_init(ui_widget_t *widget, const ui_widget_ops_t *ops);
void ui_widget_set_bounds(ui_widget_t *widget, int x, int y, int width, int height);
void ui_widget_set_visible(ui_widget_t *widget, bool visible);
void ui_widget_set_user_data(ui_widget_t *widget, void *user_data);
void *ui_widget_user_data(const ui_widget_t *widget);

bool ui_widget_add_child(ui_widget_t *parent, ui_widget_t *child);
void ui_widget_remove_child(ui_widget_t *child);

void ui_widget_render_tree(ui_widget_t *root, ui_context_t *ctx);
bool ui_widget_dispatch_event(ui_widget_t *root, const ui_event_t *event);
void ui_widget_destroy_tree(ui_widget_t *root);

void ui_widget_set_style(ui_widget_t *widget, const ui_style_t *style);
const ui_style_t *ui_widget_style(const ui_widget_t *widget);

#endif
