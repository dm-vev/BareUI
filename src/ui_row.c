#include "ui_row.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ui_widget_t *widget;
    bool expand;
    const char *key;
} ui_row_child_t;

struct ui_row {
    ui_widget_t base;
    ui_main_axis_alignment_t alignment;
    ui_cross_axis_alignment_t vertical_alignment;
    ui_main_axis_alignment_t run_alignment;
    ui_row_scroll_mode_t scroll_mode;
    bool auto_scroll;
    bool wrap;
    bool tight;
    bool rtl;
    int spacing;
    int run_spacing;
    int on_scroll_interval;
    int scroll_offset;
    int max_scroll_offset;
    ui_row_child_t *children;
    size_t child_count;
    size_t child_capacity;
};

static const int SCROLL_INTERVAL_DEFAULT = 10;

static bool ui_row_ensure_capacity(ui_row_t *row, size_t target)
{
    if (row->child_capacity >= target) {
        return true;
    }
    size_t next = row->child_capacity ? row->child_capacity * 2 : 4;
    if (next < target) {
        next = target;
    }
    ui_row_child_t *realloced = realloc(row->children, next * sizeof(*row->children));
    if (!realloced) {
        return false;
    }
    row->children = realloced;
    row->child_capacity = next;
    return true;
}

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

static const ui_style_t *ui_widget_style_safe(const ui_widget_t *widget)
{
    static const ui_style_t default_style = {0};
    const ui_style_t *style = ui_widget_style(widget);
    return style ? style : &default_style;
}

static int ui_row_clamp_scroll(ui_row_t *row, int value)
{
    if (!row || row->scroll_mode == UI_SCROLL_DIRECTION_NONE) {
        return 0;
    }
    if (value < 0) {
        return 0;
    }
    if (value > row->max_scroll_offset) {
        return row->max_scroll_offset;
    }
    return value;
}

static void ui_row_update_scroll_limits(ui_row_t *row, int content_width, int available)
{
    if (!row) {
        return;
    }
    if (available <= 0) {
        row->max_scroll_offset = content_width;
    } else if (content_width > available) {
        row->max_scroll_offset = content_width - available;
    } else {
        row->max_scroll_offset = 0;
    }
    row->scroll_offset = ui_row_clamp_scroll(row, row->scroll_offset);
    if (row->auto_scroll) {
        row->scroll_offset = row->max_scroll_offset;
    }
}

static void ui_row_layout(ui_row_t *row, const ui_rect_t *bounds)
{
    if (!row || !bounds) {
        return;
    }
    const ui_style_t *style = ui_widget_style_safe(&row->base);
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

    int total_child_width = 0;
    size_t expand_count = 0;
    for (size_t i = 0; i < row->child_count; ++i) {
        ui_widget_t *child = row->children[i].widget;
        const ui_style_t *child_style = ui_widget_style_safe(child);
        int child_width = child->bounds.width;
        if (child_width <= 0) {
            child_width = 0;
        }
        if (row->children[i].expand) {
            expand_count++;
        }
        total_child_width += child_width + child_style->margin_left + child_style->margin_right;
    }

    int spacing_total = 0;
    if (row->alignment == UI_MAIN_AXIS_START || row->alignment == UI_MAIN_AXIS_END ||
        row->alignment == UI_MAIN_AXIS_CENTER) {
        if (row->child_count > 1) {
            spacing_total = row->spacing * ((int)row->child_count - 1);
        }
    }

    int expand_extra = 0;
    int base_width = total_child_width;
    if (expand_count > 0) {
        int remaining = content_width - base_width - spacing_total;
        if (remaining > 0) {
            expand_extra = remaining / (int)expand_count;
        }
    }

    int used_space = total_child_width + spacing_total + expand_extra * (int)expand_count;
    ui_row_update_scroll_limits(row, used_space, content_width);

    int start_x = content_x;
    int gap = 0;
    if (row->child_count > 0) {
        switch (row->alignment) {
        case UI_MAIN_AXIS_START:
            gap = row->spacing;
            break;
        case UI_MAIN_AXIS_END:
            start_x = content_x + content_width - used_space;
            gap = row->spacing;
            break;
        case UI_MAIN_AXIS_CENTER:
            start_x = content_x + (content_width - used_space) / 2;
            gap = row->spacing;
            break;
        case UI_MAIN_AXIS_SPACE_BETWEEN: {
            int gaps = row->child_count > 1 ? (int)row->child_count - 1 : 0;
            if (gaps > 0) {
                int available = content_width - total_child_width - expand_extra * (int)expand_count;
                gap = available > 0 ? available / gaps : 0;
            }
            break;
        }
        case UI_MAIN_AXIS_SPACE_AROUND: {
            int available = content_width - total_child_width - expand_extra * (int)expand_count;
            gap = row->child_count > 0 ? (available > 0 ? available / (int)row->child_count : 0) : 0;
            start_x += gap / 2;
            break;
        }
        case UI_MAIN_AXIS_SPACE_EVENLY: {
            int available = content_width - total_child_width - expand_extra * (int)expand_count;
            gap = row->child_count > 0 ? (available > 0 ? available / ((int)row->child_count + 1) : 0) : 0;
            start_x += gap;
            break;
        }
        }
    }

    start_x -= row->scroll_offset;

    int cursor_x = start_x;
    for (size_t i = 0; i < row->child_count; ++i) {
        ui_widget_t *child = row->children[i].widget;
        const ui_style_t *child_style = ui_widget_style_safe(child);
        child->bounds.width = child->bounds.width > 0 ? child->bounds.width : 0;
        if (row->children[i].expand) {
            child->bounds.width += expand_extra;
        }

        int child_height = child->bounds.height;
        if (child_height <= 0 || row->vertical_alignment == UI_CROSS_AXIS_STRETCH) {
            child_height = content_height - child_style->margin_top - child_style->margin_bottom;
            if (child_height < 0) {
                child_height = 0;
            }
        }
        child->bounds.height = child_height;

        int y = content_y + child_style->margin_top;
        if (row->vertical_alignment == UI_CROSS_AXIS_CENTER) {
            y = content_y + (content_height - child_height) / 2;
        } else if (row->vertical_alignment == UI_CROSS_AXIS_END) {
            y = content_y + content_height - child_height - child_style->margin_bottom;
        }
        if (row->rtl) {
            // When RTL, reverse x placement.
            cursor_x = content_x + content_width - (cursor_x - content_x) - child->bounds.width;
        }

        cursor_x += child_style->margin_left;
        child->bounds.x = cursor_x;
        child->bounds.y = y;
        cursor_x += child->bounds.width + child_style->margin_right;
        if (row->alignment == UI_MAIN_AXIS_SPACE_AROUND || row->alignment == UI_MAIN_AXIS_SPACE_EVENLY) {
            cursor_x += gap;
        } else if (row->alignment == UI_MAIN_AXIS_START ||
                   row->alignment == UI_MAIN_AXIS_CENTER ||
                   row->alignment == UI_MAIN_AXIS_END) {
            cursor_x += gap;
        } else if (row->alignment == UI_MAIN_AXIS_SPACE_BETWEEN) {
            cursor_x += gap;
        }
    }
}

static bool ui_row_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    ui_row_t *row = (ui_row_t *)widget;
    if (!row || !bounds) {
        return false;
    }
    const ui_style_t *style = ui_widget_style_safe(widget);
    ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                         style->background_color);
    ui_row_layout(row, bounds);
    return true;
}

static const ui_widget_ops_t ui_row_ops = {
    .render = ui_row_render,
    .handle_event = NULL,
    .destroy = NULL
};

ui_row_t *ui_row_create(void)
{
    ui_row_t *row = calloc(1, sizeof(*row));
    if (!row) {
        return NULL;
    }
    ui_row_init(row);
    return row;
}

void ui_row_destroy(ui_row_t *row)
{
    if (!row) {
        return;
    }
    if (row->children) {
        free(row->children);
    }
    free(row);
}

void ui_row_init(ui_row_t *row)
{
    if (!row) {
        return;
    }
    memset(row, 0, sizeof(*row));
    ui_widget_init(&row->base, &ui_row_ops);
    row->alignment = UI_MAIN_AXIS_START;
    row->vertical_alignment = UI_CROSS_AXIS_START;
    row->run_alignment = UI_MAIN_AXIS_START;
    row->scroll_mode = UI_SCROLL_DIRECTION_NONE;
    row->spacing = 10;
    row->run_spacing = 10;
    row->on_scroll_interval = SCROLL_INTERVAL_DEFAULT;
}

bool ui_row_add_control(ui_row_t *row, ui_widget_t *control, bool expand, const char *key)
{
    if (!row || !control) {
        return false;
    }
    if (!ui_row_ensure_capacity(row, row->child_count + 1)) {
        return false;
    }
    row->children[row->child_count++] = (ui_row_child_t){
        .widget = control,
        .expand = expand,
        .key = key
    };
    ui_widget_add_child_last(&row->base, control);
    if (row->auto_scroll) {
        row->scroll_offset = row->max_scroll_offset;
    }
    return true;
}

bool ui_row_remove_control(ui_row_t *row, ui_widget_t *control)
{
    if (!row || !control) {
        return false;
    }
    for (size_t i = 0; i < row->child_count; ++i) {
        if (row->children[i].widget == control) {
            ui_widget_remove_child(control);
            memmove(&row->children[i], &row->children[i + 1],
                    (row->child_count - i - 1) * sizeof(ui_row_child_t));
            --row->child_count;
            return true;
        }
    }
    return false;
}

size_t ui_row_control_count(const ui_row_t *row)
{
    return row ? row->child_count : 0;
}

void ui_row_set_alignment(ui_row_t *row, ui_main_axis_alignment_t alignment)
{
    if (row) {
        row->alignment = alignment;
    }
}

ui_main_axis_alignment_t ui_row_alignment(const ui_row_t *row)
{
    return row ? row->alignment : UI_MAIN_AXIS_START;
}

void ui_row_set_vertical_alignment(ui_row_t *row, ui_cross_axis_alignment_t alignment)
{
    if (row) {
        row->vertical_alignment = alignment;
    }
}

ui_cross_axis_alignment_t ui_row_vertical_alignment(const ui_row_t *row)
{
    return row ? row->vertical_alignment : UI_CROSS_AXIS_START;
}

void ui_row_set_spacing(ui_row_t *row, int spacing)
{
    if (row && spacing >= 0) {
        row->spacing = spacing;
    }
}

int ui_row_spacing(const ui_row_t *row)
{
    return row ? row->spacing : 0;
}

void ui_row_set_auto_scroll(ui_row_t *row, bool auto_scroll)
{
    if (row) {
        row->auto_scroll = auto_scroll;
        if (auto_scroll) {
            row->scroll_offset = row->max_scroll_offset;
        }
    }
}

bool ui_row_auto_scroll(const ui_row_t *row)
{
    return row ? row->auto_scroll : false;
}

void ui_row_set_scroll_mode(ui_row_t *row, ui_row_scroll_mode_t mode)
{
    if (row) {
        row->scroll_mode = mode;
        if (mode == UI_SCROLL_DIRECTION_NONE) {
            row->scroll_offset = 0;
        } else {
            row->scroll_offset = ui_row_clamp_scroll(row, row->scroll_offset);
        }
    }
}

ui_row_scroll_mode_t ui_row_scroll_mode(const ui_row_t *row)
{
    return row ? row->scroll_mode : UI_SCROLL_DIRECTION_NONE;
}

void ui_row_set_scroll_offset(ui_row_t *row, int offset)
{
    if (row) {
        row->scroll_offset = ui_row_clamp_scroll(row, offset);
    }
}

int ui_row_scroll_offset(const ui_row_t *row)
{
    return row ? row->scroll_offset : 0;
}

void ui_row_set_wrap(ui_row_t *row, bool wrap)
{
    if (row) {
        row->wrap = wrap;
    }
}

bool ui_row_wrap(const ui_row_t *row)
{
    return row ? row->wrap : false;
}

void ui_row_set_run_alignment(ui_row_t *row, ui_main_axis_alignment_t alignment)
{
    if (row) {
        row->run_alignment = alignment;
    }
}

ui_main_axis_alignment_t ui_row_run_alignment(const ui_row_t *row)
{
    return row ? row->run_alignment : UI_MAIN_AXIS_START;
}

void ui_row_set_run_spacing(ui_row_t *row, int spacing)
{
    if (row && spacing >= 0) {
        row->run_spacing = spacing;
    }
}

int ui_row_run_spacing(const ui_row_t *row)
{
    return row ? row->run_spacing : 0;
}

void ui_row_set_tight(ui_row_t *row, bool tight)
{
    if (row) {
        row->tight = tight;
    }
}

bool ui_row_tight(const ui_row_t *row)
{
    return row ? row->tight : false;
}

void ui_row_set_rtl(ui_row_t *row, bool rtl)
{
    if (row) {
        row->rtl = rtl;
    }
}

bool ui_row_rtl(const ui_row_t *row)
{
    return row ? row->rtl : false;
}

void ui_row_set_on_scroll_interval(ui_row_t *row, int interval_ms)
{
    if (row) {
        row->on_scroll_interval = interval_ms >= 0 ? interval_ms : row->on_scroll_interval;
    }
}

int ui_row_on_scroll_interval(const ui_row_t *row)
{
    return row ? row->on_scroll_interval : SCROLL_INTERVAL_DEFAULT;
}

static int ui_row_find_child_with_key(const ui_row_t *row, const char *key)
{
    if (!row || !key) {
        return -1;
    }
    for (size_t i = 0; i < row->child_count; ++i) {
        if (row->children[i].key && strcmp(row->children[i].key, key) == 0) {
            return (int)i;
        }
    }
    return -1;
}

void ui_row_scroll_to(ui_row_t *row, int offset, int delta, const char *key,
                      int duration_ms, double curve)
{
    (void)duration_ms;
    (void)curve;
    if (!row) {
        return;
    }
    int target = row->scroll_offset;
    if (key) {
        int idx = ui_row_find_child_with_key(row, key);
        if (idx >= 0) {
            target = row->children[idx].widget->bounds.x - row->base.bounds.x;
        }
    }
    if (offset != INT_MIN) {
        if (offset < 0) {
            target = row->max_scroll_offset + offset + 1;
        } else {
            target = offset;
        }
    }
    target += delta;
    row->scroll_offset = ui_row_clamp_scroll(row, target);
}

const ui_widget_t *ui_row_widget(const ui_row_t *row)
{
    return row ? &row->base : NULL;
}

ui_widget_t *ui_row_widget_mutable(ui_row_t *row)
{
    return row ? &row->base : NULL;
}
