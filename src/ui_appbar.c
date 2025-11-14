#include "ui_appbar.h"

#include "ui_widget.h"

#include <stdlib.h>
#include <string.h>

#define UI_APPBAR_DEFAULT_LEADING_WIDTH 56
#define UI_APPBAR_DEFAULT_TITLE_SPACING 16
#define UI_APPBAR_DEFAULT_TOOLBAR_HEIGHT 56
#define UI_APPBAR_DEFAULT_TOOLBAR_OPACITY 1.0
#define UI_APPBAR_DEFAULT_BACKGROUND ui_color_from_hex(0xF6F7F8)
#define UI_APPBAR_DEFAULT_CONTENT_COLOR ui_color_from_hex(0x212121)
#define UI_APPBAR_DEFAULT_SHADOW_COLOR ui_color_from_hex(0x000000)
#define UI_APPBAR_DEFAULT_SURFACE_TINT ui_color_from_hex(0x000000)

typedef struct {
    ui_widget_t *widget;
    const char *key;
} ui_appbar_action_t;

struct ui_appbar {
    ui_widget_t base;
    ui_widget_t *leading;
    ui_widget_t *title;
    ui_appbar_action_t *actions;
    size_t action_count;
    size_t action_capacity;
    bool adaptive;
    bool automatically_imply_leading;
    bool center_title;
    bool force_material_transparency;
    bool is_secondary;
    bool exclude_header_semantics;
    double elevation;
    double elevation_on_scroll;
    double toolbar_opacity;
    int leading_width;
    int title_spacing;
    int toolbar_height;
    ui_color_t bgcolor;
    ui_color_t content_color;
    ui_color_t shadow_color;
    ui_color_t surface_tint_color;
    ui_appbar_clip_behavior_t clip_behavior;
};

static const ui_style_t *ui_widget_style_safe(const ui_widget_t *widget)
{
    static const ui_style_t default_style = {
        .border_sides = UI_BORDER_LEFT | UI_BORDER_TOP,
        .border_width = 0,
        .border_color = 0,
        .box_shadow = {.enabled = false},
        .custom_count = 0,
        .flags = 0
    };
    if (!widget) {
        return &default_style;
    }
    const ui_style_t *style = ui_widget_style(widget);
    return style ? style : &default_style;
}

static bool ui_appbar_ensure_capacity(ui_appbar_t *appbar, size_t target)
{
    if (!appbar) {
        return false;
    }
    if (appbar->action_capacity >= target) {
        return true;
    }
    size_t next = appbar->action_capacity ? appbar->action_capacity * 2 : 4;
    if (next < target) {
        next = target;
    }
    ui_appbar_action_t *realloced = realloc(appbar->actions, next * sizeof(*appbar->actions));
    if (!realloced) {
        return false;
    }
    appbar->actions = realloced;
    appbar->action_capacity = next;
    return true;
}

static int ui_appbar_resolve_height(const ui_widget_t *widget, int content_height)
{
    if (!widget) {
        return 0;
    }
    const ui_style_t *style = ui_widget_style_safe(widget);
    int available = content_height - style->margin_top - style->margin_bottom;
    if (available < 0) {
        available = 0;
    }
    int height = widget->bounds.height > 0 ? widget->bounds.height : available;
    if (height > available) {
        height = available;
    }
    return height;
}

static int ui_appbar_resolve_action_width(ui_widget_t *widget, int content_height)
{
    if (!widget) {
        return 0;
    }
    const ui_style_t *style = ui_widget_style_safe(widget);
    int fallback = content_height - style->margin_top - style->margin_bottom;
    if (fallback < 0) {
        fallback = 0;
    }
    int width = widget->bounds.width > 0 ? widget->bounds.width : fallback;
    if (width < 0) {
        width = 0;
    }
    return width;
}

static int ui_appbar_measure_actions_total(const ui_appbar_t *appbar, int content_height)
{
    if (!appbar || appbar->action_count == 0) {
        return 0;
    }
    int total = 0;
    for (size_t i = 0; i < appbar->action_count; ++i) {
        ui_widget_t *action = appbar->actions[i].widget;
        if (!action) {
            continue;
        }
        const ui_style_t *style = ui_widget_style_safe(action);
        int width = ui_appbar_resolve_action_width(action, content_height);
        total += width + style->margin_left + style->margin_right;
    }
    return total;
}

static void ui_appbar_layout(ui_appbar_t *appbar, const ui_rect_t *bounds)
{
    if (!appbar || !bounds) {
        return;
    }
    const ui_style_t *style = ui_widget_style_safe(&appbar->base);
    int reserved_height = bounds->height;
    if (appbar->toolbar_height > 0) {
        reserved_height = appbar->toolbar_height < bounds->height ? appbar->toolbar_height : bounds->height;
    }
    ui_rect_t layout_bounds = *bounds;
    if (reserved_height < bounds->height) {
        layout_bounds.y += (bounds->height - reserved_height) / 2;
        layout_bounds.height = reserved_height;
    }

    int content_x = layout_bounds.x + style->padding_left;
    int content_y = layout_bounds.y + style->padding_top;
    int content_width = layout_bounds.width - style->padding_left - style->padding_right;
    int content_height = layout_bounds.height - style->padding_top - style->padding_bottom;
    if (content_width < 0) {
        content_width = 0;
    }
    if (content_height < 0) {
        content_height = 0;
    }

    int reserved_leading = appbar->leading_width > 0 ? appbar->leading_width : UI_APPBAR_DEFAULT_LEADING_WIDTH;
    if (reserved_leading < 0) {
        reserved_leading = 0;
    }
    int spacing = appbar->title_spacing >= 0 ? appbar->title_spacing : UI_APPBAR_DEFAULT_TITLE_SPACING;
    int title_start = content_x + reserved_leading + spacing;
    if (title_start > content_x + content_width) {
        title_start = content_x + content_width;
    }

    if (appbar->leading) {
        const ui_style_t *leading_style = ui_widget_style_safe(appbar->leading);
        int slot = reserved_leading - leading_style->margin_left - leading_style->margin_right;
        if (slot < 0) {
            slot = 0;
        }
        int width = appbar->leading->bounds.width > 0 ? appbar->leading->bounds.width : slot;
        if (width > slot) {
            width = slot;
        }
        int height = ui_appbar_resolve_height(appbar->leading, content_height);
        int y = content_y + leading_style->margin_top;
        int extra = content_height - leading_style->margin_top - leading_style->margin_bottom - height;
        if (extra > 0) {
            y += extra / 2;
        }
        int x = content_x + leading_style->margin_left;
        if (slot > width) {
            x += (slot - width) / 2;
        }
        appbar->leading->bounds.x = x;
        appbar->leading->bounds.y = y;
        appbar->leading->bounds.width = width;
        appbar->leading->bounds.height = height;
    }

    int actions_spacing = appbar->action_count > 0 ? spacing : 0;
    int total_actions = ui_appbar_measure_actions_total(appbar, content_height);
    int layout_cursor = content_x + content_width;
    if (total_actions > content_width) {
        layout_cursor = content_x + content_width;
    }

    for (size_t i = appbar->action_count; i > 0; --i) {
        ui_widget_t *action = appbar->actions[i - 1].widget;
        if (!action) {
            continue;
        }
        const ui_style_t *action_style = ui_widget_style_safe(action);
        int width = ui_appbar_resolve_action_width(action, content_height);
        layout_cursor -= action_style->margin_right;
        int action_x = layout_cursor - width;
        if (action_x < content_x) {
            action_x = content_x;
        }
        action->bounds.x = action_x;
        action->bounds.width = width;
        int height = ui_appbar_resolve_height(action, content_height);
        int y = content_y + action_style->margin_top;
        int extra = content_height - action_style->margin_top - action_style->margin_bottom - height;
        if (extra > 0) {
            y += extra / 2;
        }
        action->bounds.y = y;
        action->bounds.height = height;
        layout_cursor = action_x - action_style->margin_left;
        if (layout_cursor < content_x) {
            layout_cursor = content_x;
        }
    }

    int title_end = layout_cursor - actions_spacing;
    if (title_end < content_x) {
        title_end = content_x;
    }
    if (title_end > content_x + content_width) {
        title_end = content_x + content_width;
    }
    if (title_start > title_end) {
        title_start = title_end;
    }

    if (appbar->title) {
        const ui_style_t *title_style = ui_widget_style_safe(appbar->title);
        int horizontal_margins = title_style->margin_left + title_style->margin_right;
        int available = title_end - title_start - horizontal_margins;
        if (available < 0) {
            available = 0;
        }
        int width = appbar->title->bounds.width > 0 ? appbar->title->bounds.width : available;
        if (width > available) {
            width = available;
        }
        int x_base = title_start + title_style->margin_left;
        if (appbar->center_title && width > 0) {
            int center = content_x + (content_width - width) / 2;
            int left_limit = title_start + title_style->margin_left;
            int right_limit = title_end - title_style->margin_right - width;
            if (right_limit < left_limit) {
                right_limit = left_limit;
            }
            if (center < left_limit) {
                center = left_limit;
            }
            if (center > right_limit) {
                center = right_limit;
            }
            appbar->title->bounds.x = center;
        } else {
            appbar->title->bounds.x = x_base;
        }
        int available_height = content_height - title_style->margin_top - title_style->margin_bottom;
        if (available_height < 0) {
            available_height = 0;
        }
        int height = appbar->title->bounds.height > 0 ? appbar->title->bounds.height : available_height;
        if (height > available_height) {
            height = available_height;
        }
        int y = content_y + title_style->margin_top;
        int extra = available_height - height;
        if (extra > 0) {
            y += extra / 2;
        }
        appbar->title->bounds.y = y;
        appbar->title->bounds.height = height;
        appbar->title->bounds.width = width;
    }
}

static bool ui_appbar_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    ui_appbar_t *appbar = (ui_appbar_t *)widget;
    if (!appbar || !bounds) {
        return false;
    }
    if (!appbar->force_material_transparency) {
        ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                             appbar->bgcolor);
    }
    ui_appbar_layout(appbar, bounds);
    return true;
}

static const ui_widget_ops_t ui_appbar_ops = {
    .render = ui_appbar_render,
    .handle_event = NULL,
    .destroy = NULL
};

ui_appbar_t *ui_appbar_create(void)
{
    ui_appbar_t *appbar = calloc(1, sizeof(*appbar));
    if (!appbar) {
        return NULL;
    }
    ui_appbar_init(appbar);
    return appbar;
}

void ui_appbar_destroy(ui_appbar_t *appbar)
{
    if (!appbar) {
        return;
    }
    if (appbar->actions) {
        free(appbar->actions);
    }
    free(appbar);
}

void ui_appbar_init(ui_appbar_t *appbar)
{
    if (!appbar) {
        return;
    }
    memset(appbar, 0, sizeof(*appbar));
    ui_widget_init(&appbar->base, &ui_appbar_ops);
    appbar->toolbar_height = UI_APPBAR_DEFAULT_TOOLBAR_HEIGHT;
    appbar->toolbar_opacity = UI_APPBAR_DEFAULT_TOOLBAR_OPACITY;
    appbar->leading_width = UI_APPBAR_DEFAULT_LEADING_WIDTH;
    appbar->title_spacing = UI_APPBAR_DEFAULT_TITLE_SPACING;
    appbar->bgcolor = UI_APPBAR_DEFAULT_BACKGROUND;
    appbar->content_color = UI_APPBAR_DEFAULT_CONTENT_COLOR;
    appbar->shadow_color = UI_APPBAR_DEFAULT_SHADOW_COLOR;
    appbar->surface_tint_color = UI_APPBAR_DEFAULT_SURFACE_TINT;
    appbar->clip_behavior = UI_APPBAR_CLIP_BEHAVIOR_NONE;
}

const ui_widget_t *ui_appbar_widget(const ui_appbar_t *appbar)
{
    return appbar ? &appbar->base : NULL;
}

ui_widget_t *ui_appbar_widget_mutable(ui_appbar_t *appbar)
{
    return appbar ? &appbar->base : NULL;
}

bool ui_appbar_set_leading(ui_appbar_t *appbar, ui_widget_t *leading)
{
    if (!appbar) {
        return false;
    }
    if (appbar->leading == leading) {
        return true;
    }
    if (appbar->leading) {
        ui_widget_remove_child(appbar->leading);
    }
    appbar->leading = leading;
    if (leading) {
        ui_widget_add_child(&appbar->base, leading);
    }
    return true;
}

ui_widget_t *ui_appbar_leading(const ui_appbar_t *appbar)
{
    return appbar ? appbar->leading : NULL;
}

bool ui_appbar_set_title(ui_appbar_t *appbar, ui_widget_t *title)
{
    if (!appbar) {
        return false;
    }
    if (appbar->title == title) {
        return true;
    }
    if (appbar->title) {
        ui_widget_remove_child(appbar->title);
    }
    appbar->title = title;
    if (title) {
        ui_widget_add_child(&appbar->base, title);
    }
    return true;
}

ui_widget_t *ui_appbar_title(const ui_appbar_t *appbar)
{
    return appbar ? appbar->title : NULL;
}

bool ui_appbar_add_action(ui_appbar_t *appbar, ui_widget_t *action, const char *key)
{
    if (!appbar || !action) {
        return false;
    }
    if (!ui_appbar_ensure_capacity(appbar, appbar->action_count + 1)) {
        return false;
    }
    appbar->actions[appbar->action_count++] = (ui_appbar_action_t){
        .widget = action,
        .key = key
    };
    ui_widget_add_child(&appbar->base, action);
    return true;
}

bool ui_appbar_remove_action(ui_appbar_t *appbar, ui_widget_t *action)
{
    if (!appbar || !action) {
        return false;
    }
    for (size_t i = 0; i < appbar->action_count; ++i) {
        if (appbar->actions[i].widget == action) {
            ui_widget_remove_child(action);
            memmove(&appbar->actions[i], &appbar->actions[i + 1],
                    (appbar->action_count - i - 1) * sizeof(ui_appbar_action_t));
            --appbar->action_count;
            return true;
        }
    }
    return false;
}

bool ui_appbar_remove_action_by_key(ui_appbar_t *appbar, const char *key)
{
    if (!appbar || !key) {
        return false;
    }
    for (size_t i = 0; i < appbar->action_count; ++i) {
        if (appbar->actions[i].key && strcmp(appbar->actions[i].key, key) == 0) {
            ui_widget_remove_child(appbar->actions[i].widget);
            memmove(&appbar->actions[i], &appbar->actions[i + 1],
                    (appbar->action_count - i - 1) * sizeof(ui_appbar_action_t));
            --appbar->action_count;
            return true;
        }
    }
    return false;
}

size_t ui_appbar_action_count(const ui_appbar_t *appbar)
{
    return appbar ? appbar->action_count : 0;
}

void ui_appbar_clear_actions(ui_appbar_t *appbar)
{
    if (!appbar) {
        return;
    }
    for (size_t i = 0; i < appbar->action_count; ++i) {
        ui_widget_remove_child(appbar->actions[i].widget);
    }
    appbar->action_count = 0;
}

void ui_appbar_set_adaptive(ui_appbar_t *appbar, bool adaptive)
{
    if (appbar) {
        appbar->adaptive = adaptive;
    }
}

bool ui_appbar_adaptive(const ui_appbar_t *appbar)
{
    return appbar ? appbar->adaptive : false;
}

void ui_appbar_set_automatically_imply_leading(ui_appbar_t *appbar, bool imply)
{
    if (appbar) {
        appbar->automatically_imply_leading = imply;
    }
}

bool ui_appbar_automatically_imply_leading(const ui_appbar_t *appbar)
{
    return appbar ? appbar->automatically_imply_leading : false;
}

void ui_appbar_set_center_title(ui_appbar_t *appbar, bool center)
{
    if (appbar) {
        appbar->center_title = center;
    }
}

bool ui_appbar_center_title(const ui_appbar_t *appbar)
{
    return appbar ? appbar->center_title : false;
}

void ui_appbar_set_title_spacing(ui_appbar_t *appbar, int spacing)
{
    if (appbar) {
        appbar->title_spacing = spacing >= 0 ? spacing : UI_APPBAR_DEFAULT_TITLE_SPACING;
    }
}

int ui_appbar_title_spacing(const ui_appbar_t *appbar)
{
    return appbar ? appbar->title_spacing : UI_APPBAR_DEFAULT_TITLE_SPACING;
}

void ui_appbar_set_leading_width(ui_appbar_t *appbar, int width)
{
    if (appbar) {
        appbar->leading_width = width >= 0 ? width : UI_APPBAR_DEFAULT_LEADING_WIDTH;
    }
}

int ui_appbar_leading_width(const ui_appbar_t *appbar)
{
    return appbar ? appbar->leading_width : UI_APPBAR_DEFAULT_LEADING_WIDTH;
}

void ui_appbar_set_toolbar_height(ui_appbar_t *appbar, int height)
{
    if (appbar) {
        appbar->toolbar_height = height >= 0 ? height : UI_APPBAR_DEFAULT_TOOLBAR_HEIGHT;
    }
}

int ui_appbar_toolbar_height(const ui_appbar_t *appbar)
{
    return appbar ? appbar->toolbar_height : UI_APPBAR_DEFAULT_TOOLBAR_HEIGHT;
}

void ui_appbar_set_toolbar_opacity(ui_appbar_t *appbar, double opacity)
{
    if (appbar) {
        if (opacity < 0.0) {
            opacity = 0.0;
        } else if (opacity > 1.0) {
            opacity = 1.0;
        }
        appbar->toolbar_opacity = opacity;
    }
}

double ui_appbar_toolbar_opacity(const ui_appbar_t *appbar)
{
    return appbar ? appbar->toolbar_opacity : UI_APPBAR_DEFAULT_TOOLBAR_OPACITY;
}

void ui_appbar_set_background_color(ui_appbar_t *appbar, ui_color_t color)
{
    if (appbar) {
        appbar->bgcolor = color;
    }
}

ui_color_t ui_appbar_background_color(const ui_appbar_t *appbar)
{
    return appbar ? appbar->bgcolor : UI_APPBAR_DEFAULT_BACKGROUND;
}

void ui_appbar_set_content_color(ui_appbar_t *appbar, ui_color_t color)
{
    if (appbar) {
        appbar->content_color = color;
    }
}

ui_color_t ui_appbar_content_color(const ui_appbar_t *appbar)
{
    return appbar ? appbar->content_color : UI_APPBAR_DEFAULT_CONTENT_COLOR;
}

void ui_appbar_set_force_material_transparency(ui_appbar_t *appbar, bool force)
{
    if (appbar) {
        appbar->force_material_transparency = force;
    }
}

bool ui_appbar_force_material_transparency(const ui_appbar_t *appbar)
{
    return appbar ? appbar->force_material_transparency : false;
}

void ui_appbar_set_elevation(ui_appbar_t *appbar, double elevation)
{
    if (appbar) {
        appbar->elevation = elevation;
    }
}

double ui_appbar_elevation(const ui_appbar_t *appbar)
{
    return appbar ? appbar->elevation : 0.0;
}

void ui_appbar_set_elevation_on_scroll(ui_appbar_t *appbar, double elevation)
{
    if (appbar) {
        appbar->elevation_on_scroll = elevation;
    }
}

double ui_appbar_elevation_on_scroll(const ui_appbar_t *appbar)
{
    return appbar ? appbar->elevation_on_scroll : 0.0;
}

void ui_appbar_set_shadow_color(ui_appbar_t *appbar, ui_color_t color)
{
    if (appbar) {
        appbar->shadow_color = color;
    }
}

ui_color_t ui_appbar_shadow_color(const ui_appbar_t *appbar)
{
    return appbar ? appbar->shadow_color : UI_APPBAR_DEFAULT_SHADOW_COLOR;
}

void ui_appbar_set_surface_tint_color(ui_appbar_t *appbar, ui_color_t color)
{
    if (appbar) {
        appbar->surface_tint_color = color;
    }
}

ui_color_t ui_appbar_surface_tint_color(const ui_appbar_t *appbar)
{
    return appbar ? appbar->surface_tint_color : UI_APPBAR_DEFAULT_SURFACE_TINT;
}

void ui_appbar_set_clip_behavior(ui_appbar_t *appbar, ui_appbar_clip_behavior_t behavior)
{
    if (appbar) {
        appbar->clip_behavior = behavior;
    }
}

ui_appbar_clip_behavior_t ui_appbar_clip_behavior(const ui_appbar_t *appbar)
{
    return appbar ? appbar->clip_behavior : UI_APPBAR_CLIP_BEHAVIOR_NONE;
}

void ui_appbar_set_exclude_header_semantics(ui_appbar_t *appbar, bool exclude)
{
    if (appbar) {
        appbar->exclude_header_semantics = exclude;
    }
}

bool ui_appbar_exclude_header_semantics(const ui_appbar_t *appbar)
{
    return appbar ? appbar->exclude_header_semantics : false;
}

void ui_appbar_set_is_secondary(ui_appbar_t *appbar, bool secondary)
{
    if (appbar) {
        appbar->is_secondary = secondary;
    }
}

bool ui_appbar_is_secondary(const ui_appbar_t *appbar)
{
    return appbar ? appbar->is_secondary : false;
}
