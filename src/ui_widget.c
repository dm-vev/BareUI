#include "ui_widget.h"

void ui_widget_init(ui_widget_t *widget, const ui_widget_ops_t *ops)
{
    if (!widget) {
        return;
    }
    widget->parent = NULL;
    widget->first_child = NULL;
    widget->next_sibling = NULL;
    widget->ops = ops;
    widget->bounds.x = 0;
    widget->bounds.y = 0;
    widget->bounds.width = 0;
    widget->bounds.height = 0;
    widget->user_data = NULL;
    widget->visible = true;
}

void ui_widget_set_bounds(ui_widget_t *widget, int x, int y, int width, int height)
{
    if (!widget) {
        return;
    }
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.width = width > 0 ? width : 0;
    widget->bounds.height = height > 0 ? height : 0;
}

void ui_widget_set_visible(ui_widget_t *widget, bool visible)
{
    if (widget) {
        widget->visible = visible;
    }
}

void ui_widget_set_user_data(ui_widget_t *widget, void *user_data)
{
    if (widget) {
        widget->user_data = user_data;
    }
}

void *ui_widget_user_data(const ui_widget_t *widget)
{
    return widget ? widget->user_data : NULL;
}

bool ui_widget_add_child(ui_widget_t *parent, ui_widget_t *child)
{
    if (!parent || !child || parent == child) {
        return false;
    }
    ui_widget_remove_child(child);
    child->next_sibling = parent->first_child;
    parent->first_child = child;
    child->parent = parent;
    return true;
}

void ui_widget_remove_child(ui_widget_t *child)
{
    if (!child || !child->parent) {
        return;
    }

    ui_widget_t **slot = &child->parent->first_child;
    while (*slot) {
        if (*slot == child) {
            *slot = child->next_sibling;
            break;
        }
        slot = &(*slot)->next_sibling;
    }
    child->parent = NULL;
    child->next_sibling = NULL;
}

void ui_widget_render_tree(ui_widget_t *root, ui_context_t *ctx)
{
    if (!root || !ctx || !root->visible) {
        return;
    }
    if (root->ops && root->ops->render) {
        root->ops->render(ctx, root, &root->bounds);
    }
    for (ui_widget_t *child = root->first_child; child; child = child->next_sibling) {
        ui_widget_render_tree(child, ctx);
    }
}

bool ui_widget_dispatch_event(ui_widget_t *root, const ui_event_t *event)
{
    if (!root || !event || !root->visible) {
        return false;
    }
    for (ui_widget_t *child = root->first_child; child; child = child->next_sibling) {
        if (ui_widget_dispatch_event(child, event)) {
            return true;
        }
    }
    if (root->ops && root->ops->handle_event) {
        return root->ops->handle_event(root, event);
    }
    return false;
}

void ui_widget_destroy_tree(ui_widget_t *root)
{
    if (!root) {
        return;
    }
    ui_widget_t *child = root->first_child;
    while (child) {
        ui_widget_t *next = child->next_sibling;
        ui_widget_destroy_tree(child);
        child = next;
    }
    if (root->ops && root->ops->destroy) {
        root->ops->destroy(root);
    }
    ui_widget_remove_child(root);
}
