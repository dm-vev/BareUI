#include "ui_column.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ui_widget_t *widget;
    bool expand;
    const char *key;
} ui_column_child_t;

struct ui_column {
    ui_widget_t base;
    ui_main_axis_alignment_t alignment;
    ui_cross_axis_alignment_t horizontal_alignment;
    ui_main_axis_alignment_t run_alignment;
    ui_scroll_mode_t scroll_mode;
    bool auto_scroll;
    bool wrap;
    bool tight;
    bool rtl;
    int spacing;
    int run_spacing;
    int on_scroll_interval;
    int scroll_offset;
    int max_scroll_offset;
    ui_column_child_t *children;
    size_t child_count;
    size_t child_capacity;
};

static const int SCROLL_INTERVAL_DEFAULT = 10;

static bool ui_widget_add_child_last(ui_widget_t *parent, ui_widget_t *child)
{
    if (!parent || !child || parent == child) {
        return false;
    }
    ui_widget_remove_child(child);
    child->parent = parent;
    child->next_sibling = NULL;
    if (!parent->first_child) {
        parent->first_child = child;
        return true;
    }
    ui_widget_t *tail = parent->first_child;
    while (tail->next_sibling) {
        tail = tail->next_sibling;
    }
    tail->next_sibling = child;
    return true;
}

static bool ui_column_ensure_capacity(ui_column_t *column, size_t target)
{
    if (column->child_capacity >= target) {
        return true;
    }
    size_t next = column->child_capacity ? column->child_capacity * 2 : 4;
    if (next < target) {
        next = target;
    }
    ui_column_child_t *realloced = realloc(column->children, next * sizeof(*column->children));
    if (!realloced) {
        return false;
    }
    column->children = realloced;
    column->child_capacity = next;
    return true;
}

static const ui_style_t *ui_widget_style_safe(const ui_widget_t *widget)
{
    static const ui_style_t default_style = {
        .background_color = 0,
        .foreground_color = 0,
        .border_color = 0,
        .accent_color = 0,
        .border_width = 0,
        .border_radius = 0,
        .padding_left = 0,
        .padding_right = 0,
        .padding_top = 0,
        .padding_bottom = 0,
        .margin_left = 0,
        .margin_right = 0,
        .margin_top = 0,
        .margin_bottom = 0,
        .shadow_enabled = false,
        .shadow_offset_x = 0,
        .shadow_offset_y = 0,
        .shadow_color = 0,
        .extra = NULL,
        .extra_size = 0,
        .custom_count = 0
    };
    const ui_style_t *style = ui_widget_style(widget);
    return style ? style : &default_style;
}

static int ui_column_clamp_scroll(ui_column_t *column, int value)
{
    if (!column) {
        return value;
    }
    if (column->scroll_mode == UI_SCROLL_MODE_NONE) {
        return 0;
    }
    if (value < 0) {
        return 0;
    }
    if (value > column->max_scroll_offset) {
        return column->max_scroll_offset;
    }
    return value;
}

static void ui_column_update_scroll_limits(ui_column_t *column, int content_height, int available)
{
    if (!column) {
        return;
    }
    if (available <= 0) {
        column->max_scroll_offset = content_height;
    } else if (content_height > available) {
        column->max_scroll_offset = content_height - available;
    } else {
        column->max_scroll_offset = 0;
    }
    column->scroll_offset = ui_column_clamp_scroll(column, column->scroll_offset);
    if (column->auto_scroll) {
        column->scroll_offset = column->max_scroll_offset;
    }
}

static void ui_column_layout(ui_column_t *column, const ui_rect_t *bounds)
{
    if (!column || !bounds) {
        return;
    }
    const ui_style_t *style = ui_widget_style_safe(&column->base);
    int content_x = bounds->x + style->padding_left;
    int content_y = bounds->y + style->padding_top;
    int content_width = bounds->width - style->padding_left - style->padding_right;
    int content_height = bounds->height - style->padding_top - style->padding_bottom;
    if (content_width < 0) {
        content_width = 0;
    }
    if (content_height < 0) {
        content_height = 0;
    }

    int total_child_height = 0;
    size_t expand_count = 0;
    for (size_t i = 0; i < column->child_count; ++i) {
        ui_widget_t *child = column->children[i].widget;
        const ui_style_t *child_style = ui_widget_style_safe(child);
        int child_height = child->bounds.height;
        if (child_height <= 0) {
            child_height = 0;
        }
        if (column->children[i].expand) {
            expand_count++;
        }
        total_child_height += child_height + child_style->margin_top + child_style->margin_bottom;
    }

    int spacing_total = 0;
    if (column->alignment == UI_MAIN_AXIS_START || column->alignment == UI_MAIN_AXIS_END ||
        column->alignment == UI_MAIN_AXIS_CENTER) {
        if (column->child_count > 1) {
            spacing_total = column->spacing * ((int)column->child_count - 1);
        }
    }

    int expand_extra = 0;
    int base_height = total_child_height;
    if (expand_count > 0) {
        int remaining = content_height - base_height - spacing_total;
        if (remaining > 0) {
            expand_extra = remaining / (int)expand_count;
        }
    }

    int used_space = total_child_height + spacing_total + expand_extra * (int)expand_count;
    ui_column_update_scroll_limits(column, used_space, content_height);

    int start_y = content_y;
    int gap = 0;
    if (column->child_count > 0) {
        switch (column->alignment) {
        case UI_MAIN_AXIS_START:
            gap = column->spacing;
            break;
        case UI_MAIN_AXIS_END:
            start_y = content_y + content_height - used_space;
            gap = column->spacing;
            break;
        case UI_MAIN_AXIS_CENTER:
            start_y = content_y + (content_height - used_space) / 2;
            gap = column->spacing;
            break;
        case UI_MAIN_AXIS_SPACE_BETWEEN: {
            int gaps = column->child_count > 1 ? (int)column->child_count - 1 : 0;
            if (gaps > 0) {
                int available = content_height - total_child_height - expand_extra * (int)expand_count;
                gap = available > 0 ? available / gaps : 0;
            }
            break;
        }
        case UI_MAIN_AXIS_SPACE_AROUND: {
            int available = content_height - total_child_height - expand_extra * (int)expand_count;
            gap = column->child_count > 0 ? (available > 0 ? available / (int)column->child_count : 0) : 0;
            start_y += gap / 2;
            break;
        }
        case UI_MAIN_AXIS_SPACE_EVENLY: {
            int available = content_height - total_child_height - expand_extra * (int)expand_count;
            gap = column->child_count > 0 ? (available > 0 ? available / ((int)column->child_count + 1) : 0) : 0;
            start_y += gap;
            break;
        }
        }
    }

    start_y -= column->scroll_offset;

    int cursor_y = start_y;
    for (size_t i = 0; i < column->child_count; ++i) {
        ui_widget_t *child = column->children[i].widget;
        const ui_style_t *child_style = ui_widget_style_safe(child);
        child->bounds.height = child->bounds.height > 0 ? child->bounds.height : 0;
        if (column->children[i].expand) {
            child->bounds.height += expand_extra;
        }

        int child_width = child->bounds.width;
        if (child_width <= 0 || column->horizontal_alignment == UI_CROSS_AXIS_STRETCH) {
            child_width = content_width - child_style->margin_left - child_style->margin_right;
            if (child_width < 0) {
                child_width = 0;
            }
        }
        child->bounds.width = child_width;

        int x = content_x + child_style->margin_left;
        if (column->horizontal_alignment == UI_CROSS_AXIS_CENTER) {
            x = content_x + (content_width - child_width) / 2;
        } else if (column->horizontal_alignment == UI_CROSS_AXIS_END) {
            x = content_x + content_width - child_width - child_style->margin_right;
        }
        if (column->rtl) {
            x = content_x + content_width - (x - content_x) - child_width;
        }

        cursor_y += child_style->margin_top;
        child->bounds.x = x;
        child->bounds.y = cursor_y;
        cursor_y += child->bounds.height + child_style->margin_bottom;
        if (column->alignment == UI_MAIN_AXIS_SPACE_AROUND || column->alignment == UI_MAIN_AXIS_SPACE_EVENLY) {
            cursor_y += gap;
        } else if (column->alignment == UI_MAIN_AXIS_START ||
                   column->alignment == UI_MAIN_AXIS_CENTER ||
                   column->alignment == UI_MAIN_AXIS_END) {
            cursor_y += gap;
        } else if (column->alignment == UI_MAIN_AXIS_SPACE_BETWEEN) {
            cursor_y += gap;
        }
    }
}

static bool ui_column_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    ui_column_t *column = (ui_column_t *)widget;
    if (!column || !bounds) {
        return false;
    }
    const ui_style_t *style = ui_widget_style_safe(widget);
    ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                         style->background_color);
    ui_column_layout(column, bounds);
    return true;
}

static const ui_widget_ops_t ui_column_ops = {
    .render = ui_column_render,
    .handle_event = NULL,
    .destroy = NULL
};

ui_column_t *ui_column_create(void)
{
    ui_column_t *column = calloc(1, sizeof(*column));
    if (!column) {
        return NULL;
    }
    ui_column_init(column);
    return column;
}

void ui_column_destroy(ui_column_t *column)
{
    if (!column) {
        return;
    }
    if (column->children) {
        free(column->children);
    }
    free(column);
}

void ui_column_init(ui_column_t *column)
{
    if (!column) {
        return;
    }
    memset(column, 0, sizeof(*column));
    ui_widget_init(&column->base, &ui_column_ops);
    column->alignment = UI_MAIN_AXIS_START;
    column->horizontal_alignment = UI_CROSS_AXIS_START;
    column->run_alignment = UI_MAIN_AXIS_START;
    column->scroll_mode = UI_SCROLL_MODE_NONE;
    column->spacing = 10;
    column->run_spacing = 10;
    column->on_scroll_interval = SCROLL_INTERVAL_DEFAULT;
    column->scroll_offset = 0;
    column->auto_scroll = false;
}

bool ui_column_add_control(ui_column_t *column, ui_widget_t *control, bool expand, const char *key)
{
    if (!column || !control) {
        return false;
    }
    if (!ui_column_ensure_capacity(column, column->child_count + 1)) {
        return false;
    }
    column->children[column->child_count++] = (ui_column_child_t){
        .widget = control,
        .expand = expand,
        .key = key
    };
    ui_widget_add_child_last(&column->base, control);
    if (column->auto_scroll) {
        column->scroll_offset = column->max_scroll_offset;
    }
    return true;
}

bool ui_column_remove_control(ui_column_t *column, ui_widget_t *control)
{
    if (!column || !control) {
        return false;
    }
    for (size_t i = 0; i < column->child_count; ++i) {
        if (column->children[i].widget == control) {
            ui_widget_remove_child(control);
            memmove(&column->children[i], &column->children[i + 1],
                    (column->child_count - i - 1) * sizeof(ui_column_child_t));
            --column->child_count;
            return true;
        }
    }
    return false;
}

size_t ui_column_control_count(const ui_column_t *column)
{
    return column ? column->child_count : 0;
}

void ui_column_set_alignment(ui_column_t *column, ui_main_axis_alignment_t alignment)
{
    if (column) {
        column->alignment = alignment;
    }
}

ui_main_axis_alignment_t ui_column_alignment(const ui_column_t *column)
{
    return column ? column->alignment : UI_MAIN_AXIS_START;
}

void ui_column_set_horizontal_alignment(ui_column_t *column,
                                        ui_cross_axis_alignment_t alignment)
{
    if (column) {
        column->horizontal_alignment = alignment;
    }
}

ui_cross_axis_alignment_t ui_column_horizontal_alignment(const ui_column_t *column)
{
    return column ? column->horizontal_alignment : UI_CROSS_AXIS_START;
}

void ui_column_set_spacing(ui_column_t *column, int spacing)
{
    if (column && spacing >= 0) {
        column->spacing = spacing;
    }
}

int ui_column_spacing(const ui_column_t *column)
{
    return column ? column->spacing : 0;
}

void ui_column_set_auto_scroll(ui_column_t *column, bool auto_scroll)
{
    if (column) {
        column->auto_scroll = auto_scroll;
        if (auto_scroll) {
            column->scroll_offset = column->max_scroll_offset;
        }
    }
}

bool ui_column_auto_scroll(const ui_column_t *column)
{
    return column ? column->auto_scroll : false;
}

void ui_column_set_scroll_mode(ui_column_t *column, ui_scroll_mode_t mode)
{
    if (column) {
        column->scroll_mode = mode;
        if (mode == UI_SCROLL_MODE_NONE) {
            column->scroll_offset = 0;
        } else {
            column->scroll_offset = ui_column_clamp_scroll(column, column->scroll_offset);
        }
    }
}

ui_scroll_mode_t ui_column_scroll_mode(const ui_column_t *column)
{
    return column ? column->scroll_mode : UI_SCROLL_MODE_NONE;
}

void ui_column_set_scroll_offset(ui_column_t *column, int offset)
{
    if (column) {
        column->scroll_offset = ui_column_clamp_scroll(column, offset);
    }
}

int ui_column_scroll_offset(const ui_column_t *column)
{
    return column ? column->scroll_offset : 0;
}

void ui_column_set_wrap(ui_column_t *column, bool wrap)
{
    if (column) {
        column->wrap = wrap;
    }
}

bool ui_column_wrap(const ui_column_t *column)
{
    return column ? column->wrap : false;
}

void ui_column_set_run_alignment(ui_column_t *column, ui_main_axis_alignment_t alignment)
{
    if (column) {
        column->run_alignment = alignment;
    }
}

ui_main_axis_alignment_t ui_column_run_alignment(const ui_column_t *column)
{
    return column ? column->run_alignment : UI_MAIN_AXIS_START;
}

void ui_column_set_run_spacing(ui_column_t *column, int spacing)
{
    if (column && spacing >= 0) {
        column->run_spacing = spacing;
    }
}

int ui_column_run_spacing(const ui_column_t *column)
{
    return column ? column->run_spacing : 0;
}

void ui_column_set_wrapping_spacing(ui_column_t *column, int spacing)
{
    if (column && spacing >= 0) {
        column->run_spacing = spacing;
    }
}

void ui_column_set_tight(ui_column_t *column, bool tight)
{
    if (column) {
        column->tight = tight;
    }
}

bool ui_column_tight(const ui_column_t *column)
{
    return column ? column->tight : false;
}

void ui_column_set_rtl(ui_column_t *column, bool rtl)
{
    if (column) {
        column->rtl = rtl;
    }
}

bool ui_column_rtl(const ui_column_t *column)
{
    return column ? column->rtl : false;
}

void ui_column_set_on_scroll_interval(ui_column_t *column, int interval_ms)
{
    if (column) {
        column->on_scroll_interval = interval_ms >= 0 ? interval_ms : column->on_scroll_interval;
    }
}

int ui_column_on_scroll_interval(const ui_column_t *column)
{
    return column ? column->on_scroll_interval : SCROLL_INTERVAL_DEFAULT;
}

static int ui_column_find_child_index_with_key(const ui_column_t *column, const char *key)
{
    if (!column || !key) {
        return -1;
    }
    for (size_t i = 0; i < column->child_count; ++i) {
        if (column->children[i].key && strcmp(column->children[i].key, key) == 0) {
            return (int)i;
        }
    }
    return -1;
}

void ui_column_scroll_to(ui_column_t *column, int offset, int delta, const char *key,
                         int duration_ms, double curve)
{
    (void)duration_ms;
    (void)curve;
    if (!column) {
        return;
    }
    int target = column->scroll_offset;
    if (key) {
        int index = ui_column_find_child_index_with_key(column, key);
        if (index >= 0) {
            target = column->children[index].widget->bounds.y - column->base.bounds.y;
        }
    }
    if (offset != INT_MIN) {
        if (offset < 0) {
            target = column->max_scroll_offset + offset + 1;
        } else {
            target = offset;
        }
    }
    target += delta;
    column->scroll_offset = ui_column_clamp_scroll(column, target);
}

const ui_widget_t *ui_column_widget(const ui_column_t *column)
{
    return column ? &column->base : NULL;
}

ui_widget_t *ui_column_widget_mutable(ui_column_t *column)
{
    return column ? &column->base : NULL;
}
