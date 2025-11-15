#include "ui_widget.h"

#include <string.h>

static bool ui_style_find_prop_idx(const ui_style_t *style, const char *key, size_t *out)
{
    if (!style || !key) {
        return false;
    }
    for (size_t i = 0; i < style->custom_count; ++i) {
        if (style->custom_props[i].name &&
            strcmp(style->custom_props[i].name, key) == 0) {
            if (out) {
                *out = i;
            }
            return true;
        }
    }
    return false;
}

void ui_style_init(ui_style_t *style)
{
    if (!style) {
        return;
    }
    memset(style, 0, sizeof(*style));
    style->border_width = 0;
    style->border_sides = UI_BORDER_LEFT | UI_BORDER_TOP;
    style->border_color = 0;
    style->box_shadow.enabled = false;
    style->custom_count = 0;
    style->flags = 0;
}

void ui_style_copy(ui_style_t *dst, const ui_style_t *src)
{
    if (!dst || !src) {
        return;
    }
    memcpy(dst, src, sizeof(*dst));
}

bool ui_style_set_custom_prop(ui_style_t *style, const char *key, uint32_t value)
{
    if (!style || !key) {
        return false;
    }
    size_t idx;
    if (ui_style_find_prop_idx(style, key, &idx)) {
        style->custom_props[idx].value = value;
        return true;
    }
    if (style->custom_count >= UI_STYLE_MAX_CUSTOM_PROPS) {
        return false;
    }
    style->custom_props[style->custom_count].name = key;
    style->custom_props[style->custom_count].value = value;
    style->custom_count++;
    return true;
}

bool ui_style_get_custom_prop(const ui_style_t *style, const char *key, uint32_t *out)
{
    if (!style || !key || !out) {
        return false;
    }
    size_t idx;
    if (!ui_style_find_prop_idx(style, key, &idx)) {
        return false;
    }
    *out = style->custom_props[idx].value;
    return true;
}

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
    ui_style_init(&widget->style);
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

static void ui_widget_render_tree_internal(ui_widget_t *widget, ui_context_t *ctx)
{
    if (!widget || !ctx || !widget->visible) {
        return;
    }
    ui_context_push_clip(ctx, &widget->bounds);
    if (widget->ops && widget->ops->render) {
        widget->ops->render(ctx, widget, &widget->bounds);
    }
    for (ui_widget_t *child = widget->first_child; child; child = child->next_sibling) {
        ui_widget_render_tree_internal(child, ctx);
    }
    ui_context_pop_clip(ctx);
}

void ui_widget_render_tree(ui_widget_t *root, ui_context_t *ctx)
{
    ui_widget_render_tree_internal(root, ctx);
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

void ui_widget_set_style(ui_widget_t *widget, const ui_style_t *style)
{
    if (!widget) {
        return;
    }
    if (style) {
        ui_style_copy(&widget->style, style);
    } else {
        ui_style_init(&widget->style);
    }
    if (widget->ops && widget->ops->style_changed) {
        widget->ops->style_changed(widget, &widget->style);
    }
}

const ui_style_t *ui_widget_style(const ui_widget_t *widget)
{
    return widget ? &widget->style : NULL;
}
