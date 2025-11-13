#include "ui_checkbox.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "ui_primitives.h"

struct ui_checkbox {
    ui_widget_t base;
    char *label;
    const bareui_font_t *font;
    ui_checkbox_state_t state;
    bool tristate;
    bool hovered;
    bool pressed;
    bool focused;
    bool autofocus;
    bool enabled;
    bool is_error;
    ui_color_t active_color;
    ui_color_t fill_color;
    ui_color_t border_color;
    ui_color_t check_color;
    ui_color_t hover_color;
    ui_checkbox_label_position_t label_position;
    ui_checkbox_change_fn on_change;
    void *on_change_data;
    ui_checkbox_event_fn on_focus;
    void *on_focus_data;
    ui_checkbox_event_fn on_blur;
    void *on_blur_data;
};

static bool ui_checkbox_next_codepoint(const unsigned char **ptr, uint32_t *out)
{
    if (!ptr || !*ptr || **ptr == '\0') {
        return false;
    }
    unsigned char first = **ptr;
    uint32_t codepoint = first;
    size_t advance = 1;
    if (first < 0x80) {
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

static int ui_checkbox_measure_text(const ui_checkbox_t *checkbox, const char *text)
{
    if (!checkbox || !text) {
        return 0;
    }
    const bareui_font_t *font = checkbox->font ? checkbox->font : bareui_font_default();
    const unsigned char *ptr = (const unsigned char *)text;
    int width = 0;
    while (*ptr) {
        uint32_t codepoint;
        if (!ui_checkbox_next_codepoint(&ptr, &codepoint)) {
            break;
        }
        bareui_font_glyph_t glyph;
        if (bareui_font_lookup(font, codepoint, &glyph)) {
            width += glyph.spacing;
        }
    }
    return width;
}

static bool ui_checkbox_contains(const ui_checkbox_t *checkbox, int x, int y)
{
    if (!checkbox) {
        return false;
    }
    const ui_rect_t *bounds = &checkbox->base.bounds;
    return x >= bounds->x && y >= bounds->y && x < bounds->x + bounds->width &&
           y < bounds->y + bounds->height;
}

static void ui_checkbox_notify_hover(ui_checkbox_t *checkbox, bool hovering)
{
    if (!checkbox) {
        return;
    }
    checkbox->hovered = hovering;
}

static void ui_checkbox_notify_focus(ui_checkbox_t *checkbox, bool focus_state)
{
    if (!checkbox || checkbox->focused == focus_state) {
        return;
    }
    checkbox->focused = focus_state;
    if (focus_state && checkbox->on_focus) {
        checkbox->on_focus(checkbox, checkbox->on_focus_data);
    } else if (!focus_state && checkbox->on_blur) {
        checkbox->on_blur(checkbox, checkbox->on_blur_data);
    }
}

static ui_checkbox_state_t ui_checkbox_next_state(ui_checkbox_state_t current, bool tristate)
{
    if (!tristate) {
        return current == UI_CHECKBOX_STATE_CHECKED ? UI_CHECKBOX_STATE_UNCHECKED
                                                    : UI_CHECKBOX_STATE_CHECKED;
    }
    switch (current) {
    case UI_CHECKBOX_STATE_UNCHECKED:
        return UI_CHECKBOX_STATE_CHECKED;
    case UI_CHECKBOX_STATE_CHECKED:
        return UI_CHECKBOX_STATE_INDETERMINATE;
    case UI_CHECKBOX_STATE_INDETERMINATE:
    default:
        return UI_CHECKBOX_STATE_UNCHECKED;
    }
}

static void ui_checkbox_apply_style(ui_widget_t *widget, const ui_style_t *style)
{
    if (!widget || !style) {
        return;
    }
    ui_checkbox_t *checkbox = (ui_checkbox_t *)widget;
    if (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR) {
        checkbox->fill_color = style->background_color;
    }
    if (style->flags & UI_STYLE_FLAG_FOREGROUND_COLOR) {
        checkbox->check_color = style->foreground_color;
    }
    if (style->flags & UI_STYLE_FLAG_ACCENT_COLOR) {
        checkbox->active_color = style->accent_color;
    }
    if (style->flags & UI_STYLE_FLAG_BORDER_COLOR) {
        checkbox->border_color = style->border_color;
    }
}

static void ui_checkbox_set_state_internal(ui_checkbox_t *checkbox, ui_checkbox_state_t state,
                                           bool notify)
{
    if (!checkbox) {
        return;
    }
    if (!checkbox->tristate && state == UI_CHECKBOX_STATE_INDETERMINATE) {
        state = UI_CHECKBOX_STATE_UNCHECKED;
    }
    if (checkbox->state == state) {
        return;
    }
    checkbox->state = state;
    if (notify && checkbox->on_change) {
        checkbox->on_change(checkbox, state, checkbox->on_change_data);
    }
}

static bool ui_checkbox_handle_event(ui_widget_t *widget, const ui_event_t *event)
{
    ui_checkbox_t *checkbox = (ui_checkbox_t *)widget;
    if (!checkbox || !checkbox->base.visible || !checkbox->enabled) {
        return false;
    }
    switch (event->type) {
    case UI_EVENT_TOUCH_DOWN: {
        if (!ui_checkbox_contains(checkbox, event->data.touch.x, event->data.touch.y)) {
            return false;
        }
        checkbox->pressed = true;
        ui_checkbox_notify_hover(checkbox, true);
        ui_checkbox_notify_focus(checkbox, true);
        return true;
    }
    case UI_EVENT_TOUCH_MOVE: {
        bool inside = ui_checkbox_contains(checkbox, event->data.touch.x, event->data.touch.y);
        ui_checkbox_notify_hover(checkbox, inside);
        if (!inside) {
            checkbox->pressed = false;
        }
        return inside;
    }
    case UI_EVENT_TOUCH_UP: {
        bool inside = ui_checkbox_contains(checkbox, event->data.touch.x, event->data.touch.y);
        if (checkbox->pressed && inside) {
            ui_checkbox_set_state_internal(checkbox,
                                           ui_checkbox_next_state(checkbox->state, checkbox->tristate),
                                           true);
        }
        checkbox->pressed = false;
        ui_checkbox_notify_hover(checkbox, inside);
        if (!inside) {
            ui_checkbox_notify_focus(checkbox, false);
        }
        return true;
    }
    case UI_EVENT_KEY_DOWN: {
        uint32_t key = event->data.key.keycode;
        if (key == ' ' || key == '\n' || key == '\r') {
            checkbox->pressed = true;
            ui_checkbox_notify_focus(checkbox, true);
            return true;
        }
        break;
    }
    case UI_EVENT_KEY_UP: {
        uint32_t key = event->data.key.keycode;
        if ((key == ' ' || key == '\n' || key == '\r') && checkbox->pressed) {
            checkbox->pressed = false;
            ui_checkbox_set_state_internal(checkbox,
                                           ui_checkbox_next_state(checkbox->state, checkbox->tristate),
                                           true);
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

static bool ui_checkbox_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    ui_checkbox_t *checkbox = (ui_checkbox_t *)widget;
    if (!checkbox || !bounds) {
        return false;
    }
    const ui_style_t *style = ui_widget_style(widget);
    if (style && (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR)) {
        ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                             style->background_color);
    }

    int box_size = bounds->height - 4;
    if (box_size <= 0) {
        box_size = bounds->height;
    }
    if (box_size <= 0) {
        box_size = 1;
    }
    if (box_size > bounds->width) {
        box_size = bounds->width;
    }

    int label_width = checkbox->label ? ui_checkbox_measure_text(checkbox, checkbox->label) : 0;
    int label_gap = 6;
    int box_x = bounds->x + 2;
    int box_y = bounds->y + (bounds->height - box_size) / 2;
    int label_x = box_x + box_size + label_gap;

    if (checkbox->label_position == UI_CHECKBOX_LABEL_POSITION_LEFT && checkbox->label) {
        label_x = bounds->x + 2;
        box_x = bounds->x + bounds->width - box_size - 2;
        if (label_width > 0) {
            int min_box_x = label_x + label_width + label_gap;
            if (box_x < min_box_x) {
                box_x = min_box_x;
            }
        }
    }

    if (box_x + box_size > bounds->x + bounds->width) {
        box_x = bounds->x + bounds->width - box_size;
    }
    if (box_x < bounds->x) {
        box_x = bounds->x;
    }
    if (box_y < bounds->y) {
        box_y = bounds->y;
    }

    ui_color_t fill = checkbox->fill_color;
    if (checkbox->enabled && checkbox->hovered && checkbox->hover_color) {
        fill = checkbox->hover_color;
    }
    ui_color_t box_fill = fill;
    if (checkbox->state == UI_CHECKBOX_STATE_CHECKED ||
        checkbox->state == UI_CHECKBOX_STATE_INDETERMINATE) {
        box_fill = checkbox->active_color;
    }
    if (!checkbox->enabled) {
        box_fill = ui_color_from_hex(0x10121F);
    }

    ui_context_fill_rect(ctx, box_x, box_y, box_size, box_size, box_fill);

    ui_color_t border = checkbox->is_error ? ui_color_from_hex(0xD12B2B) : checkbox->border_color;
    if (!checkbox->enabled) {
        border = ui_color_from_hex(0x525257);
    }
    if (box_size >= 2) {
        ui_context_fill_rect(ctx, box_x, box_y, box_size, 1, border);
        ui_context_fill_rect(ctx, box_x, box_y + box_size - 1, box_size, 1, border);
        ui_context_fill_rect(ctx, box_x, box_y, 1, box_size, border);
        ui_context_fill_rect(ctx, box_x + box_size - 1, box_y, 1, box_size, border);
    }

    const char *icon = NULL;
    if (checkbox->state == UI_CHECKBOX_STATE_CHECKED) {
        icon = "X";
    } else if (checkbox->state == UI_CHECKBOX_STATE_INDETERMINATE) {
        icon = "-";
    }
    if (icon) {
        const bareui_font_t *font = checkbox->font ? checkbox->font : bareui_font_default();
        bareui_font_glyph_t glyph;
        int icon_width = 0;
        if (bareui_font_lookup(font, (uint32_t)icon[0], &glyph)) {
            icon_width = glyph.spacing;
        }
        if (icon_width <= 0) {
            icon_width = font ? font->height : BAREUI_FONT_HEIGHT;
        }
        int icon_x = box_x + (box_size - icon_width) / 2;
        if (icon_x < box_x) {
            icon_x = box_x;
        }
        int icon_y = box_y + (box_size - (font ? font->height : BAREUI_FONT_HEIGHT)) / 2;
        if (icon_y < box_y) {
            icon_y = box_y;
        }
        if (icon_x + icon_width > box_x + box_size) {
            icon_x = box_x + box_size - icon_width;
        }
        const bareui_font_t *prev_font = ui_context_font(ctx);
        ui_context_set_font(ctx, font);
        ui_color_t icon_color = checkbox->check_color;
        if (checkbox->is_error) {
            icon_color = ui_color_from_hex(0xD12B2B);
        } else if (!checkbox->enabled) {
            icon_color = ui_color_from_hex(0x7A7B84);
        }
        char icon_text[2] = {icon[0], '\0'};
        ui_context_draw_text(ctx, icon_x, icon_y, icon_text, icon_color);
        ui_context_set_font(ctx, prev_font);
    }

    if (checkbox->label && *checkbox->label) {
        const bareui_font_t *font = checkbox->font ? checkbox->font : bareui_font_default();
        const bareui_font_t *prev_font = ui_context_font(ctx);
        ui_context_set_font(ctx, font);
        ui_color_t label_color = ui_color_from_hex(0xF5F5F5);
        if (style && (style->flags & UI_STYLE_FLAG_FOREGROUND_COLOR)) {
            label_color = style->foreground_color;
        }
        int label_y = bounds->y + (bounds->height - (font ? font->height : BAREUI_FONT_HEIGHT)) / 2;
        if (label_y < bounds->y) {
            label_y = bounds->y;
        }
        ui_context_draw_text(ctx, label_x, label_y, checkbox->label, label_color);
        ui_context_set_font(ctx, prev_font);
    }

    return true;
}

static const ui_widget_ops_t ui_checkbox_ops = {
    .render = ui_checkbox_render,
    .handle_event = ui_checkbox_handle_event,
    .destroy = NULL,
    .style_changed = ui_checkbox_apply_style
};

static char *ui_checkbox_copy_label(const char *label)
{
    if (!label) {
        return NULL;
    }
    size_t len = strlen(label) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, label, len);
    }
    return copy;
}

ui_checkbox_t *ui_checkbox_create(void)
{
    ui_checkbox_t *checkbox = calloc(1, sizeof(*checkbox));
    if (!checkbox) {
        return NULL;
    }
    ui_checkbox_init(checkbox);
    return checkbox;
}

void ui_checkbox_destroy(ui_checkbox_t *checkbox)
{
    if (!checkbox) {
        return;
    }
    free(checkbox->label);
    free(checkbox);
}

void ui_checkbox_init(ui_checkbox_t *checkbox)
{
    if (!checkbox) {
        return;
    }
    ui_widget_init(&checkbox->base, &ui_checkbox_ops);
    checkbox->label = NULL;
    checkbox->font = bareui_font_default();
    checkbox->state = UI_CHECKBOX_STATE_UNCHECKED;
    checkbox->tristate = false;
    checkbox->hovered = false;
    checkbox->pressed = false;
    checkbox->focused = false;
    checkbox->autofocus = false;
    checkbox->enabled = true;
    checkbox->is_error = false;
    checkbox->active_color = ui_color_from_hex(0x5992FF);
    checkbox->fill_color = ui_color_from_hex(0x10121F);
    checkbox->border_color = ui_color_from_hex(0xF5F5F5);
    checkbox->check_color = ui_color_from_hex(0xF5F5F5);
    checkbox->hover_color = ui_color_from_hex(0x1F263F);
    checkbox->label_position = UI_CHECKBOX_LABEL_POSITION_RIGHT;
    checkbox->on_change = NULL;
    checkbox->on_change_data = NULL;
    checkbox->on_focus = NULL;
    checkbox->on_focus_data = NULL;
    checkbox->on_blur = NULL;
    checkbox->on_blur_data = NULL;

    ui_style_t default_style;
    ui_style_init(&default_style);
    default_style.background_color = checkbox->fill_color;
    default_style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    default_style.foreground_color = checkbox->check_color;
    default_style.flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    default_style.accent_color = checkbox->active_color;
    default_style.flags |= UI_STYLE_FLAG_ACCENT_COLOR;
    default_style.border_color = checkbox->border_color;
    default_style.flags |= UI_STYLE_FLAG_BORDER_COLOR;
    ui_widget_set_style(&checkbox->base, &default_style);
}

void ui_checkbox_set_state(ui_checkbox_t *checkbox, ui_checkbox_state_t state)
{
    ui_checkbox_set_state_internal(checkbox, state, false);
}

ui_checkbox_state_t ui_checkbox_state(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->state : UI_CHECKBOX_STATE_UNCHECKED;
}

void ui_checkbox_set_tristate(ui_checkbox_t *checkbox, bool tristate)
{
    if (!checkbox) {
        return;
    }
    checkbox->tristate = tristate;
    if (!tristate && checkbox->state == UI_CHECKBOX_STATE_INDETERMINATE) {
        checkbox->state = UI_CHECKBOX_STATE_UNCHECKED;
    }
}

bool ui_checkbox_tristate(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->tristate : false;
}

void ui_checkbox_set_active_color(ui_checkbox_t *checkbox, ui_color_t color)
{
    if (!checkbox) {
        return;
    }
    checkbox->active_color = color;
    ui_style_t *style = &checkbox->base.style;
    style->accent_color = color;
    style->flags |= UI_STYLE_FLAG_ACCENT_COLOR;
    if (checkbox->base.ops && checkbox->base.ops->style_changed) {
        checkbox->base.ops->style_changed(&checkbox->base, style);
    }
}

ui_color_t ui_checkbox_active_color(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->active_color : 0;
}

void ui_checkbox_set_fill_color(ui_checkbox_t *checkbox, ui_color_t color)
{
    if (!checkbox) {
        return;
    }
    checkbox->fill_color = color;
    ui_style_t *style = &checkbox->base.style;
    style->background_color = color;
    style->flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    if (checkbox->base.ops && checkbox->base.ops->style_changed) {
        checkbox->base.ops->style_changed(&checkbox->base, style);
    }
}

ui_color_t ui_checkbox_fill_color(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->fill_color : 0;
}

void ui_checkbox_set_border_color(ui_checkbox_t *checkbox, ui_color_t color)
{
    if (!checkbox) {
        return;
    }
    checkbox->border_color = color;
    ui_style_t *style = &checkbox->base.style;
    style->border_color = color;
    style->flags |= UI_STYLE_FLAG_BORDER_COLOR;
    if (checkbox->base.ops && checkbox->base.ops->style_changed) {
        checkbox->base.ops->style_changed(&checkbox->base, style);
    }
}

ui_color_t ui_checkbox_border_color(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->border_color : 0;
}

void ui_checkbox_set_check_color(ui_checkbox_t *checkbox, ui_color_t color)
{
    if (!checkbox) {
        return;
    }
    checkbox->check_color = color;
    ui_style_t *style = &checkbox->base.style;
    style->foreground_color = color;
    style->flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    if (checkbox->base.ops && checkbox->base.ops->style_changed) {
        checkbox->base.ops->style_changed(&checkbox->base, style);
    }
}

ui_color_t ui_checkbox_check_color(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->check_color : 0;
}

void ui_checkbox_set_hover_color(ui_checkbox_t *checkbox, ui_color_t color)
{
    if (checkbox) {
        checkbox->hover_color = color;
    }
}

ui_color_t ui_checkbox_hover_color(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->hover_color : 0;
}

void ui_checkbox_set_label(ui_checkbox_t *checkbox, const char *label)
{
    if (!checkbox) {
        return;
    }
    free(checkbox->label);
    checkbox->label = ui_checkbox_copy_label(label);
}

const char *ui_checkbox_label(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->label : NULL;
}

void ui_checkbox_set_label_position(ui_checkbox_t *checkbox,
                                    ui_checkbox_label_position_t position)
{
    if (checkbox) {
        checkbox->label_position = position;
    }
}

ui_checkbox_label_position_t ui_checkbox_label_position(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->label_position : UI_CHECKBOX_LABEL_POSITION_RIGHT;
}

void ui_checkbox_set_font(ui_checkbox_t *checkbox, const bareui_font_t *font)
{
    if (checkbox) {
        checkbox->font = font ? font : bareui_font_default();
    }
}

const bareui_font_t *ui_checkbox_font(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->font : NULL;
}

void ui_checkbox_set_enabled(ui_checkbox_t *checkbox, bool enabled)
{
    if (checkbox) {
        checkbox->enabled = enabled;
    }
}

bool ui_checkbox_enabled(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->enabled : false;
}

void ui_checkbox_set_autofocus(ui_checkbox_t *checkbox, bool autofocus)
{
    if (checkbox) {
        checkbox->autofocus = autofocus;
    }
}

bool ui_checkbox_autofocus(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->autofocus : false;
}

void ui_checkbox_set_is_error(ui_checkbox_t *checkbox, bool is_error)
{
    if (checkbox) {
        checkbox->is_error = is_error;
    }
}

bool ui_checkbox_is_error(const ui_checkbox_t *checkbox)
{
    return checkbox ? checkbox->is_error : false;
}

void ui_checkbox_set_on_change(ui_checkbox_t *checkbox, ui_checkbox_change_fn handler,
                               void *user_data)
{
    if (checkbox) {
        checkbox->on_change = handler;
        checkbox->on_change_data = user_data;
    }
}

void ui_checkbox_set_on_focus(ui_checkbox_t *checkbox, ui_checkbox_event_fn handler,
                              void *user_data)
{
    if (checkbox) {
        checkbox->on_focus = handler;
        checkbox->on_focus_data = user_data;
    }
}

void ui_checkbox_set_on_blur(ui_checkbox_t *checkbox, ui_checkbox_event_fn handler,
                             void *user_data)
{
    if (checkbox) {
        checkbox->on_blur = handler;
        checkbox->on_blur_data = user_data;
    }
}

const ui_widget_t *ui_checkbox_widget(const ui_checkbox_t *checkbox)
{
    return checkbox ? &checkbox->base : NULL;
}

ui_widget_t *ui_checkbox_widget_mutable(ui_checkbox_t *checkbox)
{
    return checkbox ? &checkbox->base : NULL;
}
