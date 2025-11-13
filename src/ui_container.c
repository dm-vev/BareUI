#include "ui_container.h"

#include <stdbool.h>
#include <stdlib.h>

static const ui_widget_ops_t ui_container_ops;

static ui_container_t *ui_container_from_widget(ui_widget_t *widget)
{
    return widget ? (ui_container_t *)widget : NULL;
}

static const ui_style_t *ui_widget_style_safe(const ui_widget_t *widget)
{
    const ui_style_t *style = ui_widget_style(widget);
    static const ui_style_t default_style = {
        .border_sides = UI_BORDER_LEFT | UI_BORDER_TOP,
        .border_width = 0,
        .border_color = 0,
        .box_shadow = {.enabled = false},
        .custom_count = 0,
        .flags = 0
    };
    return style ? style : &default_style;
}

void ui_container_layout_children(ui_container_t *container)
{
    if (!container) {
        return;
    }
    ui_widget_t *widget = &container->base;
    const ui_rect_t bounds = widget->bounds;
    const ui_style_t *style = ui_widget_style_safe(widget);

    const int content_x = bounds.x + style->padding_left;
    const int content_y = bounds.y + style->padding_top;
    int content_width = bounds.width - style->padding_left - style->padding_right;
    int content_height = bounds.height - style->padding_top - style->padding_bottom;
    if (content_width < 0) {
        content_width = 0;
    }
    if (content_height < 0) {
        content_height = 0;
    }

    int cursor_x = content_x;
    int cursor_y = content_y;

    for (ui_widget_t *child = widget->first_child; child; child = child->next_sibling) {
        const ui_style_t *child_style = ui_widget_style_safe(child);
        int margin_left = child_style->margin_left;
        int margin_right = child_style->margin_right;
        int margin_top = child_style->margin_top;
        int margin_bottom = child_style->margin_bottom;

        int child_width = child->bounds.width > 0 ? child->bounds.width
                                                  : content_width - margin_left - margin_right;
        if (child_width < 0) {
            child_width = 0;
        }
        int child_height = child->bounds.height > 0 ? child->bounds.height
                                                    : content_height - margin_top - margin_bottom;
        if (child_height < 0) {
            child_height = 0;
        }

        switch (container->layout) {
        case UI_CONTAINER_LAYOUT_HORIZONTAL:
            child->bounds.x = cursor_x + margin_left;
            child->bounds.y = content_y + margin_top;
            child->bounds.width = child_width;
            child->bounds.height = child_height;
            cursor_x += margin_left + child_width + margin_right + container->spacing;
            break;
        case UI_CONTAINER_LAYOUT_VERTICAL:
            child->bounds.x = content_x + margin_left;
            child->bounds.y = cursor_y + margin_top;
            child->bounds.width = child_width;
            child->bounds.height = child_height;
            cursor_y += margin_top + child_height + margin_bottom + container->spacing;
            break;
        case UI_CONTAINER_LAYOUT_OVERLAY:
            child->bounds.x = content_x + margin_left;
            child->bounds.y = content_y + margin_top;
            child->bounds.width = child_width;
            child->bounds.height = child_height;
            break;
        }
    }
}

static bool ui_container_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    ui_container_t *container = ui_container_from_widget(widget);
    if (!container || !bounds) {
        return false;
    }
    const ui_style_t *style = ui_widget_style_safe(widget);
    ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                         style->background_color);
    ui_container_layout_children(container);
    return true;
}

void ui_container_init(ui_container_t *container, ui_container_layout_t layout)
{
    if (!container) {
        return;
    }
    ui_widget_init(&container->base, &ui_container_ops);
    container->layout = layout;
    container->spacing = 0;
}

void ui_container_set_layout(ui_container_t *container, ui_container_layout_t layout)
{
    if (container) {
        container->layout = layout;
    }
}

ui_container_layout_t ui_container_layout(const ui_container_t *container)
{
    return container ? container->layout : UI_CONTAINER_LAYOUT_OVERLAY;
}

void ui_container_set_spacing(ui_container_t *container, int spacing)
{
    if (container) {
        container->spacing = spacing;
    }
}

int ui_container_spacing(const ui_container_t *container)
{
    return container ? container->spacing : 0;
}

static const ui_widget_ops_t ui_container_ops = {
    .render = ui_container_render,
    .handle_event = NULL,
    .destroy = NULL
};
