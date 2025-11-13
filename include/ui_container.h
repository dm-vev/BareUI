#ifndef UI_CONTAINER_H
#define UI_CONTAINER_H

#include "ui_widget.h"

typedef enum {
    UI_CONTAINER_LAYOUT_VERTICAL,
    UI_CONTAINER_LAYOUT_HORIZONTAL,
    UI_CONTAINER_LAYOUT_OVERLAY
} ui_container_layout_t;

typedef struct {
    ui_widget_t base;
    ui_container_layout_t layout;
    int spacing;
} ui_container_t;

void ui_container_init(ui_container_t *container, ui_container_layout_t layout);
void ui_container_set_layout(ui_container_t *container, ui_container_layout_t layout);
ui_container_layout_t ui_container_layout(const ui_container_t *container);
void ui_container_set_spacing(ui_container_t *container, int spacing);
int ui_container_spacing(const ui_container_t *container);

void ui_container_layout_children(ui_container_t *container);

#endif
