#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include "ui_primitives.h"

#include <stdbool.h>

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

struct ui_widget {
    ui_widget_t *parent;
    ui_widget_t *first_child;
    ui_widget_t *next_sibling;
    const ui_widget_ops_t *ops;
    ui_rect_t bounds;
    void *user_data;
    bool visible;
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

#endif
