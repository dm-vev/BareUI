#include "ui_tab.h"

#include "ui_font.h"
#include "ui_primitives.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ui_tab {
    char *text;
    ui_widget_t *icon;
    ui_widget_t *tab_content;
    ui_widget_t *content;
    ui_tabs_t *owner;
};

struct ui_tabs {
    ui_widget_t base;
    ui_tab_clip_behavior_t clip_behavior;
    bool scrollable;
    ui_tab_alignment_t tab_alignment;
    ui_padding_t padding;
    ui_padding_t label_padding;
    ui_color_t indicator_color;
    int indicator_thickness;
    int indicator_padding;
    bool indicator_tab_size;
    ui_border_radius_t indicator_border_radius;
    ui_border_side_t indicator_border_side;
    ui_color_t divider_color;
    int divider_height;
    ui_color_t label_color;
    ui_color_t unselected_label_color;
    ui_color_t overlay_color;
    bool enable_feedback;
    ui_border_radius_t splash_border_radius;
    ui_mouse_cursor_t mouse_cursor;
    int tab_spacing;
    size_t selected_index;
    size_t pressed_index;
    size_t hovered_index;
    size_t tab_count;
    size_t tab_capacity;
    ui_tab_t **tabs;
    ui_tabs_change_fn on_change;
    void *on_change_data;
    ui_tabs_click_fn on_click;
    void *on_click_data;
    int *layout_offsets;
    int *layout_widths;
    int *layout_text_widths;
    size_t layout_capacity;
    int layout_header_left;
    int layout_header_width;
    int layout_header_height;
    int layout_header_top;
    int layout_total_width;
    bool layout_dirty;
    ui_rect_t layout_bounds;
};

static const ui_padding_t UI_TABS_DEFAULT_PADDING = {.top = 4, .bottom = 4, .left = 8, .right = 8};
static const ui_padding_t UI_TABS_DEFAULT_LABEL_PADDING = {.top = 2, .bottom = 2, .left = 8, .right = 8};
static const ui_border_radius_t UI_TABS_DEFAULT_RADIUS = {.top_left = 0, .top_right = 0, .bottom_left = 0,
                                                           .bottom_right = 0};

static void ui_tabs_mark_layout_dirty(ui_tabs_t *tabs);
static void ui_tabs_update_content_visibility(ui_tabs_t *tabs);
static void ui_tabs_layout_contents(ui_tabs_t *tabs, const ui_rect_t *bounds);
static size_t ui_tabs_index_at(ui_tabs_t *tabs, const ui_rect_t *bounds, int x, int y);
static void ui_tabs_prepare_layout(ui_tabs_t *tabs, const ui_rect_t *bounds);
static void ui_tabs_compute_layout(ui_tabs_t *tabs, const ui_rect_t *bounds);

static const ui_widget_ops_t ui_tabs_ops;

static bool ui_tab_next_codepoint(const unsigned char **ptr, uint32_t *out)
{
    if (!ptr || !*ptr || **ptr == '\0') {
        return false;
    }
    unsigned char first = **ptr;
    uint32_t codepoint = first;
    size_t advance = 1;
    if (first < 0x80) {
        // ASCII
    } else if ((first & 0xE0) == 0xC0) {
        codepoint &= 0x1F;
        advance = 2;
    } else if ((first & 0xF0) == 0xE0) {
        codepoint &= 0x0F;
        advance = 3;
    } else if ((first & 0xF8) == 0xF0) {
        codepoint &= 0x07;
        advance = 4;
    } else {
        return false;
    }
    for (size_t i = 1; i < advance; ++i) {
        unsigned char c = (*ptr)[i];
        if ((c & 0xC0) != 0x80) {
            return false;
        }
        codepoint = (codepoint << 6) | (c & 0x3F);
    }
    *ptr += advance;
    if (out) {
        *out = codepoint;
    }
    return true;
}

static char *ui_tab_strdup(const char *value)
{
    if (!value) {
        return NULL;
    }
    size_t len = strlen(value) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, value, len);
    }
    return copy;
}

static int ui_tab_measure_text(const char *text)
{
    if (!text) {
        return 0;
    }
    const bareui_font_t *font = bareui_font_default();
    const unsigned char *ptr = (const unsigned char *)text;
    int width = 0;
    while (*ptr) {
        uint32_t cp;
        if (!ui_tab_next_codepoint(&ptr, &cp)) {
            break;
        }
        bareui_font_glyph_t glyph;
        if (bareui_font_lookup(font, cp, &glyph)) {
            width += glyph.spacing;
        }
    }
    return width;
}

static bool ui_tabs_ensure_tab_capacity(ui_tabs_t *tabs, size_t required)
{
    if (!tabs) {
        return false;
    }
    if (tabs->tab_capacity >= required) {
        return true;
    }
    size_t next = tabs->tab_capacity ? tabs->tab_capacity * 2 : 4;
    if (next < required) {
        next = required;
    }
    ui_tab_t **new_tabs = realloc(tabs->tabs, next * sizeof(*new_tabs));
    if (!new_tabs) {
        return false;
    }
    tabs->tabs = new_tabs;
    tabs->tab_capacity = next;
    return true;
}

static bool ui_tabs_ensure_layout_capacity(ui_tabs_t *tabs, size_t required)
{
    if (!tabs) {
        return false;
    }
    if (tabs->layout_capacity >= required) {
        return true;
    }
    size_t next = tabs->layout_capacity ? tabs->layout_capacity * 2 : 4;
    if (next < required) {
        next = required;
    }
    int *offsets = realloc(tabs->layout_offsets, next * sizeof(*offsets));
    if (!offsets) {
        return false;
    }
    int *widths = realloc(tabs->layout_widths, next * sizeof(*widths));
    if (!widths) {
        free(offsets);
        return false;
    }
    int *text_widths = realloc(tabs->layout_text_widths, next * sizeof(*text_widths));
    if (!text_widths) {
        free(offsets);
        free(widths);
        return false;
    }
    tabs->layout_offsets = offsets;
    tabs->layout_widths = widths;
    tabs->layout_text_widths = text_widths;
    tabs->layout_capacity = next;
    return true;
}

static void ui_tabs_mark_layout_dirty(ui_tabs_t *tabs)
{
    if (!tabs) {
        return;
    }
    tabs->layout_dirty = true;
    tabs->layout_bounds = (ui_rect_t){0};
}

static void ui_tabs_update_content_visibility(ui_tabs_t *tabs)
{
    if (!tabs) {
        return;
    }
    for (size_t i = 0; i < tabs->tab_count; ++i) {
        ui_tab_t *tab = tabs->tabs[i];
        if (!tab || !tab->content) {
            continue;
        }
        ui_widget_set_visible(tab->content, i == tabs->selected_index);
    }
}

static void ui_tabs_layout_contents(ui_tabs_t *tabs, const ui_rect_t *bounds)
{
    if (!tabs || !bounds) {
        return;
    }
    const ui_style_t *style = ui_widget_style(&tabs->base);
    int content_left = bounds->x + style->padding_left;
    int content_right = bounds->x + bounds->width - style->padding_right;
    int content_top = tabs->layout_header_top + tabs->layout_header_height;
    int content_bottom = bounds->y + bounds->height - style->padding_bottom;
    int content_height = content_bottom - content_top;
    if (content_height < 0) {
        content_height = 0;
    }
    int content_width = content_right - content_left;
    if (content_width < 0) {
        content_width = 0;
    }
    for (size_t i = 0; i < tabs->tab_count; ++i) {
        ui_tab_t *tab = tabs->tabs[i];
        if (!tab || !tab->content) {
            continue;
        }
        ui_widget_set_bounds(tab->content, content_left, content_top, content_width, content_height);
        ui_widget_set_visible(tab->content, i == tabs->selected_index);
    }
}

static int ui_tabs_label_area_height(const ui_tabs_t *tabs)
{
    const bareui_font_t *font = bareui_font_default();
    return tabs->label_padding.top + font->height + tabs->label_padding.bottom;
}

static void ui_tabs_compute_layout(ui_tabs_t *tabs, const ui_rect_t *bounds)
{
    if (!tabs || !bounds) {
        return;
    }
    const ui_style_t *style = ui_widget_style(&tabs->base);
    int header_left = bounds->x + style->padding_left + tabs->padding.left;
    int header_right = bounds->x + bounds->width - style->padding_right - tabs->padding.right;
    int header_width = header_right - header_left;
    if (header_width < 0) {
        header_width = 0;
    }
    tabs->layout_header_left = header_left;
    tabs->layout_header_width = header_width;
    tabs->layout_header_top = bounds->y + style->padding_top;
    int label_height = ui_tabs_label_area_height(tabs);
    tabs->layout_header_height = tabs->tab_count > 0
        ? tabs->padding.top + label_height + tabs->padding.bottom + tabs->indicator_thickness + tabs->divider_height
        : tabs->padding.top + tabs->padding.bottom + tabs->divider_height + tabs->indicator_thickness;

    if (tabs->tab_count == 0) {
        tabs->layout_total_width = 0;
        tabs->layout_dirty = false;
        return;
    }

    if (!ui_tabs_ensure_layout_capacity(tabs, tabs->tab_count)) {
        return;
    }

    int spacing = tabs->tab_spacing;
    int available = header_width;
    if (available < 0) {
        available = 0;
    }
    int total_spacing = spacing * ((int)tabs->tab_count - 1);

    int max_label_width = 0;
    for (size_t i = 0; i < tabs->tab_count; ++i) {
        ui_tab_t *tab = tabs->tabs[i];
        int measured = tab ? ui_tab_measure_text(tab->text) : 0;
        tabs->layout_text_widths[i] = measured;
        int width = measured + tabs->label_padding.left + tabs->label_padding.right;
        if (width > max_label_width) {
            max_label_width = width;
        }
    }

    int current_x = header_left;
    int total_width = 0;

    if (tabs->scrollable) {
        int sum = 0;
        for (size_t i = 0; i < tabs->tab_count; ++i) {
            int width = tabs->layout_text_widths[i] + tabs->label_padding.left + tabs->label_padding.right;
            tabs->layout_widths[i] = width;
            sum += width;
            if (i + 1 < tabs->tab_count) {
                sum += spacing;
            }
        }
        total_width = sum;
        int start_x = header_left;
        int extra = header_width - total_width;
        if (tabs->tab_alignment == UI_TAB_ALIGNMENT_CENTER && extra > 0) {
            start_x += extra / 2;
        } else if (tabs->tab_alignment == UI_TAB_ALIGNMENT_END && extra > 0) {
            start_x += extra;
        }
        current_x = start_x;
        for (size_t i = 0; i < tabs->tab_count; ++i) {
            tabs->layout_offsets[i] = current_x;
            current_x += tabs->layout_widths[i];
            if (i + 1 < tabs->tab_count) {
                current_x += spacing;
            }
        }
    } else {
        int candidate = header_width - total_spacing;
        int tab_width = tabs->tab_count > 0 ? candidate / (int)tabs->tab_count : 0;
        if (tab_width < max_label_width) {
            tab_width = max_label_width;
        }
        if (tab_width < 0) {
            tab_width = 0;
        }
        total_width = tab_width * (int)tabs->tab_count + total_spacing;
        int start_x = header_left;
        int extra = header_width - total_width;
        if (tabs->tab_alignment == UI_TAB_ALIGNMENT_CENTER && extra > 0) {
            start_x += extra / 2;
        } else if (tabs->tab_alignment == UI_TAB_ALIGNMENT_END && extra > 0) {
            start_x += extra;
        }
        current_x = start_x;
        for (size_t i = 0; i < tabs->tab_count; ++i) {
            tabs->layout_offsets[i] = current_x;
            tabs->layout_widths[i] = tab_width;
            current_x += tab_width;
            if (i + 1 < tabs->tab_count) {
                current_x += spacing;
            }
        }
    }

    tabs->layout_total_width = total_width;
    tabs->layout_dirty = false;
}

static void ui_tabs_prepare_layout(ui_tabs_t *tabs, const ui_rect_t *bounds)
{
    if (!tabs || !bounds) {
        return;
    }
    if (!tabs->layout_dirty && tabs->layout_bounds.x == bounds->x &&
        tabs->layout_bounds.y == bounds->y && tabs->layout_bounds.width == bounds->width &&
        tabs->layout_bounds.height == bounds->height) {
        return;
    }
    ui_tabs_compute_layout(tabs, bounds);
    tabs->layout_bounds = *bounds;
}

static size_t ui_tabs_index_at(ui_tabs_t *tabs, const ui_rect_t *bounds, int x, int y)
{
    if (!tabs || tabs->tab_count == 0 || !bounds) {
        return SIZE_MAX;
    }
    ui_tabs_prepare_layout(tabs, bounds);
    int top = tabs->layout_header_top;
    int bottom = top + tabs->layout_header_height;
    if (y < top || y >= bottom) {
        return SIZE_MAX;
    }
    for (size_t i = 0; i < tabs->tab_count; ++i) {
        int left = tabs->layout_offsets ? tabs->layout_offsets[i] : 0;
        int width = tabs->layout_widths ? tabs->layout_widths[i] : 0;
        if (x >= left && x < left + width) {
            return i;
        }
    }
    return SIZE_MAX;
}

static bool ui_tabs_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    if (!ctx || !widget || !bounds) {
        return false;
    }
    ui_tabs_t *tabs = (ui_tabs_t *)widget;
    const ui_style_t *style = ui_widget_style(widget);
    if (style && (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR)) {
        ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                             style->background_color);
    }
    ui_tabs_prepare_layout(tabs, bounds);
    ui_tabs_layout_contents(tabs, bounds);
    if (tabs->tab_count == 0) {
        return true;
    }
    const bareui_font_t *font = bareui_font_default();
    const bareui_font_t *prev = ui_context_font(ctx);
    ui_context_set_font(ctx, font);
    int text_y = tabs->layout_header_top + tabs->padding.top + tabs->label_padding.top;
    for (size_t i = 0; i < tabs->tab_count; ++i) {
        ui_tab_t *tab = tabs->tabs[i];
        if (!tab || !tab->text) {
            continue;
        }
        int width = tabs->layout_widths ? tabs->layout_widths[i] : 0;
        int offset = tabs->layout_offsets ? tabs->layout_offsets[i] : 0;
        int text_width = tabs->layout_text_widths ? tabs->layout_text_widths[i] : 0;
        int text_x = offset + (width - text_width) / 2;
        ui_color_t color = i == tabs->selected_index ? tabs->label_color : tabs->unselected_label_color;
        ui_context_draw_text(ctx, text_x, text_y, tab->text, color);
    }
    int indicator_y = tabs->layout_header_top + tabs->padding.top + ui_tabs_label_area_height(tabs);
    if (tabs->selected_index < tabs->tab_count) {
        int offset = tabs->layout_offsets ? tabs->layout_offsets[tabs->selected_index] : 0;
        int width = tabs->layout_widths ? tabs->layout_widths[tabs->selected_index] : 0;
        int text_width = tabs->layout_text_widths ? tabs->layout_text_widths[tabs->selected_index] : 0;
        int indicator_width = tabs->indicator_tab_size ? width : text_width;
        int indicator_x = offset + (width - indicator_width) / 2;
        indicator_x += tabs->indicator_padding;
        indicator_width -= tabs->indicator_padding * 2;
        if (indicator_width < 0) {
            indicator_width = 0;
        }
        ui_context_fill_rect(ctx, indicator_x, indicator_y, indicator_width,
                             tabs->indicator_thickness, tabs->indicator_color);
    }
    int divider_y = indicator_y + tabs->indicator_thickness;
    if (tabs->divider_height > 0) {
        ui_context_fill_rect(ctx, tabs->layout_header_left, divider_y,
                             tabs->layout_header_width, tabs->divider_height, tabs->divider_color);
    }
    ui_context_set_font(ctx, prev);
    return true;
}

static bool ui_tabs_handle_event(ui_widget_t *widget, const ui_event_t *event)
{
    if (!widget || !event) {
        return false;
    }
    ui_tabs_t *tabs = (ui_tabs_t *)widget;
    const ui_rect_t *bounds = &widget->bounds;
    switch (event->type) {
    case UI_EVENT_TOUCH_DOWN: {
        size_t idx = ui_tabs_index_at(tabs, bounds, event->data.touch.x, event->data.touch.y);
        if (idx < tabs->tab_count) {
            tabs->pressed_index = idx;
            tabs->hovered_index = idx;
            return true;
        }
        break;
    }
    case UI_EVENT_TOUCH_MOVE: {
        size_t idx = ui_tabs_index_at(tabs, bounds, event->data.touch.x, event->data.touch.y);
        if (idx < tabs->tab_count) {
            tabs->hovered_index = idx;
            return true;
        }
        if (tabs->hovered_index != SIZE_MAX) {
            tabs->hovered_index = SIZE_MAX;
            return true;
        }
        break;
    }
    case UI_EVENT_TOUCH_UP: {
        size_t idx = ui_tabs_index_at(tabs, bounds, event->data.touch.x, event->data.touch.y);
        if (idx < tabs->tab_count && tabs->pressed_index == idx) {
            ui_tabs_set_selected_index(tabs, idx);
            if (tabs->on_click) {
                tabs->on_click(tabs, idx, tabs->on_click_data);
            }
            tabs->pressed_index = SIZE_MAX;
            tabs->hovered_index = idx;
            return true;
        }
        tabs->pressed_index = SIZE_MAX;
        break;
    }
    default:
        break;
    }
    return false;
}

static void ui_tabs_destroy_internal(ui_widget_t *widget)
{
    if (!widget) {
        return;
    }
    ui_tabs_t *tabs = (ui_tabs_t *)widget;
    ui_tabs_clear(tabs);
    free(tabs->tabs);
    tabs->tabs = NULL;
    free(tabs->layout_offsets);
    tabs->layout_offsets = NULL;
    free(tabs->layout_widths);
    tabs->layout_widths = NULL;
    free(tabs->layout_text_widths);
    tabs->layout_text_widths = NULL;
    tabs->layout_capacity = 0;
}

static void ui_tabs_style_changed(ui_widget_t *widget, const ui_style_t *style)
{
    (void)style;
    ui_tabs_mark_layout_dirty((ui_tabs_t *)widget);
}

static const ui_widget_ops_t ui_tabs_ops = {
    .render = ui_tabs_render,
    .handle_event = ui_tabs_handle_event,
    .destroy = ui_tabs_destroy_internal,
    .style_changed = ui_tabs_style_changed
};

ui_tab_t *ui_tab_create(void)
{
    ui_tab_t *tab = calloc(1, sizeof(*tab));
    if (!tab) {
        return NULL;
    }
    tab->owner = NULL;
    return tab;
}

void ui_tab_destroy(ui_tab_t *tab)
{
    if (!tab) {
        return;
    }
    if (tab->text) {
        free(tab->text);
    }
    tab->text = NULL;
    tab->owner = NULL;
    free(tab);
}

void ui_tab_set_text(ui_tab_t *tab, const char *text)
{
    if (!tab) {
        return;
    }
    char *copy = ui_tab_strdup(text);
    if (tab->text) {
        free(tab->text);
    }
    tab->text = copy;
    if (tab->owner) {
        ui_tabs_mark_layout_dirty(tab->owner);
    }
}

const char *ui_tab_text(const ui_tab_t *tab)
{
    return tab ? tab->text : NULL;
}

void ui_tab_set_icon(ui_tab_t *tab, ui_widget_t *icon)
{
    if (tab) {
        tab->icon = icon;
    }
}

ui_widget_t *ui_tab_icon(const ui_tab_t *tab)
{
    return tab ? tab->icon : NULL;
}

void ui_tab_set_tab_content(ui_tab_t *tab, ui_widget_t *tab_content)
{
    if (tab) {
        tab->tab_content = tab_content;
    }
}

ui_widget_t *ui_tab_tab_content(const ui_tab_t *tab)
{
    return tab ? tab->tab_content : NULL;
}

void ui_tab_set_content(ui_tab_t *tab, ui_widget_t *content)
{
    if (!tab) {
        return;
    }
    if (tab->content && tab->owner) {
        ui_widget_remove_child(tab->content);
    }
    tab->content = content;
    if (tab->content && tab->owner) {
        ui_widget_add_child(&tab->owner->base, tab->content);
        ui_tabs_mark_layout_dirty(tab->owner);
        ui_tabs_update_content_visibility(tab->owner);
    }
}

ui_widget_t *ui_tab_content(const ui_tab_t *tab)
{
    return tab ? tab->content : NULL;
}

ui_tabs_t *ui_tabs_create(void)
{
    ui_tabs_t *tabs = calloc(1, sizeof(*tabs));
    if (!tabs) {
        return NULL;
    }
    ui_tabs_init(tabs);
    return tabs;
}

void ui_tabs_destroy(ui_tabs_t *tabs)
{
    if (!tabs) {
        return;
    }
    ui_tabs_destroy_internal(&tabs->base);
    free(tabs);
}

void ui_tabs_init(ui_tabs_t *tabs)
{
    if (!tabs) {
        return;
    }
    memset(tabs, 0, sizeof(*tabs));
    ui_widget_init(&tabs->base, &ui_tabs_ops);
    tabs->clip_behavior = UI_TAB_CLIP_BEHAVIOR_NONE;
    tabs->scrollable = false;
    tabs->tab_alignment = UI_TAB_ALIGNMENT_FILL;
    tabs->padding = UI_TABS_DEFAULT_PADDING;
    tabs->label_padding = UI_TABS_DEFAULT_LABEL_PADDING;
    tabs->indicator_color = ui_color_from_hex(0x1976D2);
    tabs->divider_color = ui_color_from_hex(0xCCCCCC);
    tabs->divider_height = 1;
    tabs->indicator_thickness = 3;
    tabs->indicator_padding = 0;
    tabs->indicator_tab_size = false;
    tabs->indicator_border_radius = UI_TABS_DEFAULT_RADIUS;
    tabs->indicator_border_side = 0;
    tabs->label_color = ui_color_from_hex(0x000000);
    tabs->unselected_label_color = ui_color_from_hex(0x444444);
    tabs->overlay_color = 0;
    tabs->enable_feedback = true;
    tabs->splash_border_radius = UI_TABS_DEFAULT_RADIUS;
    tabs->mouse_cursor = UI_MOUSE_CURSOR_DEFAULT;
    tabs->tab_spacing = 12;
    tabs->selected_index = 0;
    tabs->pressed_index = SIZE_MAX;
    tabs->hovered_index = SIZE_MAX;
    tabs->layout_bounds = (ui_rect_t){0};
    tabs->layout_dirty = true;
}

const ui_widget_t *ui_tabs_widget(const ui_tabs_t *tabs)
{
    return tabs ? &tabs->base : NULL;
}

ui_widget_t *ui_tabs_widget_mutable(ui_tabs_t *tabs)
{
    return tabs ? &tabs->base : NULL;
}

size_t ui_tabs_tab_count(const ui_tabs_t *tabs)
{
    return tabs ? tabs->tab_count : 0;
}

bool ui_tabs_add_tab(ui_tabs_t *tabs, ui_tab_t *tab)
{
    if (!tabs || !tab) {
        return false;
    }
    if (!ui_tabs_ensure_tab_capacity(tabs, tabs->tab_count + 1)) {
        return false;
    }
    tabs->tabs[tabs->tab_count] = tab;
    tab->owner = tabs;
    if (tab->content) {
        ui_widget_add_child(&tabs->base, tab->content);
    }
    tabs->tab_count++;
    tabs->pressed_index = SIZE_MAX;
    tabs->hovered_index = SIZE_MAX;
    ui_tabs_mark_layout_dirty(tabs);
    if (tabs->tab_count == 1) {
        tabs->selected_index = 0;
    }
    ui_tabs_update_content_visibility(tabs);
    return true;
}

bool ui_tabs_remove_tab(ui_tabs_t *tabs, ui_tab_t *tab)
{
    if (!tabs || !tab || tabs->tab_count == 0) {
        return false;
    }
    size_t idx = SIZE_MAX;
    for (size_t i = 0; i < tabs->tab_count; ++i) {
        if (tabs->tabs[i] == tab) {
            idx = i;
            break;
        }
    }
    if (idx == SIZE_MAX) {
        return false;
    }
    if (tab->content) {
        ui_widget_remove_child(tab->content);
    }
    tab->owner = NULL;
    for (size_t i = idx + 1; i < tabs->tab_count; ++i) {
        tabs->tabs[i - 1] = tabs->tabs[i];
    }
    tabs->tab_count--;
    ui_tab_destroy(tab);
    if (tabs->tab_count == 0) {
        tabs->selected_index = 0;
    } else if (tabs->selected_index >= tabs->tab_count) {
        tabs->selected_index = tabs->tab_count - 1;
        if (tabs->on_change) {
            tabs->on_change(tabs, tabs->selected_index, tabs->on_change_data);
        }
    }
    tabs->pressed_index = SIZE_MAX;
    tabs->hovered_index = SIZE_MAX;
    ui_tabs_mark_layout_dirty(tabs);
    ui_tabs_update_content_visibility(tabs);
    return true;
}

void ui_tabs_clear(ui_tabs_t *tabs)
{
    if (!tabs) {
        return;
    }
    for (size_t i = 0; i < tabs->tab_count; ++i) {
        ui_tab_t *tab = tabs->tabs[i];
        if (!tab) {
            continue;
        }
        if (tab->content) {
            ui_widget_remove_child(tab->content);
        }
        tab->owner = NULL;
        ui_tab_destroy(tab);
    }
    tabs->tab_count = 0;
    tabs->selected_index = 0;
    tabs->pressed_index = SIZE_MAX;
    tabs->hovered_index = SIZE_MAX;
    ui_tabs_mark_layout_dirty(tabs);
}

void ui_tabs_set_selected_index(ui_tabs_t *tabs, size_t index)
{
    if (!tabs) {
        return;
    }
    if (tabs->tab_count == 0) {
        tabs->selected_index = 0;
        return;
    }
    size_t clamped = index >= tabs->tab_count ? tabs->tab_count - 1 : index;
    if (tabs->selected_index == clamped) {
        return;
    }
    tabs->selected_index = clamped;
    ui_tabs_update_content_visibility(tabs);
    if (tabs->on_change) {
        tabs->on_change(tabs, clamped, tabs->on_change_data);
    }
}

size_t ui_tabs_selected_index(const ui_tabs_t *tabs)
{
    return tabs ? tabs->selected_index : 0;
}

void ui_tabs_set_clip_behavior(ui_tabs_t *tabs, ui_tab_clip_behavior_t behavior)
{
    if (tabs) {
        tabs->clip_behavior = behavior;
    }
}

ui_tab_clip_behavior_t ui_tabs_clip_behavior(const ui_tabs_t *tabs)
{
    return tabs ? tabs->clip_behavior : UI_TAB_CLIP_BEHAVIOR_NONE;
}

void ui_tabs_set_scrollable(ui_tabs_t *tabs, bool scrollable)
{
    if (tabs) {
        tabs->scrollable = scrollable;
        ui_tabs_mark_layout_dirty(tabs);
    }
}

bool ui_tabs_scrollable(const ui_tabs_t *tabs)
{
    return tabs ? tabs->scrollable : false;
}

void ui_tabs_set_tab_alignment(ui_tabs_t *tabs, ui_tab_alignment_t alignment)
{
    if (tabs) {
        tabs->tab_alignment = alignment;
        ui_tabs_mark_layout_dirty(tabs);
    }
}

ui_tab_alignment_t ui_tabs_tab_alignment(const ui_tabs_t *tabs)
{
    return tabs ? tabs->tab_alignment : UI_TAB_ALIGNMENT_FILL;
}

void ui_tabs_set_padding(ui_tabs_t *tabs, ui_padding_t padding)
{
    if (tabs) {
        tabs->padding = padding;
        ui_tabs_mark_layout_dirty(tabs);
    }
}

ui_padding_t ui_tabs_padding(const ui_tabs_t *tabs)
{
    return tabs ? tabs->padding : (ui_padding_t){0, 0, 0, 0};
}

void ui_tabs_set_label_padding(ui_tabs_t *tabs, ui_padding_t padding)
{
    if (tabs) {
        tabs->label_padding = padding;
        ui_tabs_mark_layout_dirty(tabs);
    }
}

ui_padding_t ui_tabs_label_padding(const ui_tabs_t *tabs)
{
    return tabs ? tabs->label_padding : (ui_padding_t){0, 0, 0, 0};
}

void ui_tabs_set_indicator_color(ui_tabs_t *tabs, ui_color_t color)
{
    if (tabs) {
        tabs->indicator_color = color;
    }
}

ui_color_t ui_tabs_indicator_color(const ui_tabs_t *tabs)
{
    return tabs ? tabs->indicator_color : 0;
}

void ui_tabs_set_indicator_thickness(ui_tabs_t *tabs, int thickness)
{
    if (tabs) {
        tabs->indicator_thickness = thickness;
        ui_tabs_mark_layout_dirty(tabs);
    }
}

int ui_tabs_indicator_thickness(const ui_tabs_t *tabs)
{
    return tabs ? tabs->indicator_thickness : 0;
}

void ui_tabs_set_indicator_padding(ui_tabs_t *tabs, int padding)
{
    if (tabs) {
        tabs->indicator_padding = padding;
    }
}

int ui_tabs_indicator_padding(const ui_tabs_t *tabs)
{
    return tabs ? tabs->indicator_padding : 0;
}

void ui_tabs_set_indicator_tab_size(ui_tabs_t *tabs, bool full_tab)
{
    if (tabs) {
        tabs->indicator_tab_size = full_tab;
    }
}

bool ui_tabs_indicator_tab_size(const ui_tabs_t *tabs)
{
    return tabs ? tabs->indicator_tab_size : false;
}

void ui_tabs_set_indicator_border_radius(ui_tabs_t *tabs, ui_border_radius_t radius)
{
    if (tabs) {
        tabs->indicator_border_radius = radius;
    }
}

ui_border_radius_t ui_tabs_indicator_border_radius(const ui_tabs_t *tabs)
{
    return tabs ? tabs->indicator_border_radius : UI_TABS_DEFAULT_RADIUS;
}

void ui_tabs_set_indicator_border_side(ui_tabs_t *tabs, ui_border_side_t sides)
{
    if (tabs) {
        tabs->indicator_border_side = sides;
    }
}

ui_border_side_t ui_tabs_indicator_border_side(const ui_tabs_t *tabs)
{
    return tabs ? tabs->indicator_border_side : 0;
}

void ui_tabs_set_divider_color(ui_tabs_t *tabs, ui_color_t color)
{
    if (tabs) {
        tabs->divider_color = color;
    }
}

ui_color_t ui_tabs_divider_color(const ui_tabs_t *tabs)
{
    return tabs ? tabs->divider_color : 0;
}

void ui_tabs_set_divider_height(ui_tabs_t *tabs, int height)
{
    if (tabs) {
        tabs->divider_height = height;
        ui_tabs_mark_layout_dirty(tabs);
    }
}

int ui_tabs_divider_height(const ui_tabs_t *tabs)
{
    return tabs ? tabs->divider_height : 0;
}

void ui_tabs_set_label_color(ui_tabs_t *tabs, ui_color_t color)
{
    if (tabs) {
        tabs->label_color = color;
    }
}

ui_color_t ui_tabs_label_color(const ui_tabs_t *tabs)
{
    return tabs ? tabs->label_color : 0;
}

void ui_tabs_set_unselected_label_color(ui_tabs_t *tabs, ui_color_t color)
{
    if (tabs) {
        tabs->unselected_label_color = color;
    }
}

ui_color_t ui_tabs_unselected_label_color(const ui_tabs_t *tabs)
{
    return tabs ? tabs->unselected_label_color : 0;
}

void ui_tabs_set_overlay_color(ui_tabs_t *tabs, ui_color_t color)
{
    if (tabs) {
        tabs->overlay_color = color;
    }
}

ui_color_t ui_tabs_overlay_color(const ui_tabs_t *tabs)
{
    return tabs ? tabs->overlay_color : 0;
}

void ui_tabs_set_enable_feedback(ui_tabs_t *tabs, bool enabled)
{
    if (tabs) {
        tabs->enable_feedback = enabled;
    }
}

bool ui_tabs_enable_feedback(const ui_tabs_t *tabs)
{
    return tabs ? tabs->enable_feedback : false;
}

void ui_tabs_set_splash_border_radius(ui_tabs_t *tabs, ui_border_radius_t radius)
{
    if (tabs) {
        tabs->splash_border_radius = radius;
    }
}

ui_border_radius_t ui_tabs_splash_border_radius(const ui_tabs_t *tabs)
{
    return tabs ? tabs->splash_border_radius : UI_TABS_DEFAULT_RADIUS;
}

void ui_tabs_set_mouse_cursor(ui_tabs_t *tabs, ui_mouse_cursor_t cursor)
{
    if (tabs) {
        tabs->mouse_cursor = cursor;
    }
}

ui_mouse_cursor_t ui_tabs_mouse_cursor(const ui_tabs_t *tabs)
{
    return tabs ? tabs->mouse_cursor : UI_MOUSE_CURSOR_DEFAULT;
}

void ui_tabs_set_on_change(ui_tabs_t *tabs, ui_tabs_change_fn handler, void *user_data)
{
    if (tabs) {
        tabs->on_change = handler;
        tabs->on_change_data = user_data;
    }
}

void ui_tabs_set_on_click(ui_tabs_t *tabs, ui_tabs_click_fn handler, void *user_data)
{
    if (tabs) {
        tabs->on_click = handler;
        tabs->on_click_data = user_data;
    }
}
