#include "ui_text.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ui_font.h"

struct ui_text {
    ui_widget_t base;
    char *value;
    ui_color_t color;
    ui_color_t background_color;
    const bareui_font_t *font;
    ui_text_align_t align;
    ui_text_overflow_t overflow;
    bool no_wrap;
    bool rtl;
    bool italic;
    int max_lines;
    int line_spacing;
};

struct ui_text_utf8_reader {
    const char *ptr;
    size_t remaining;
};


static void ui_text_apply_style(ui_widget_t *widget, const ui_style_t *style)
{
    if (!widget || !style) {
        return;
    }
    ui_text_t *text = (ui_text_t *)widget;
    if (style->flags & UI_STYLE_FLAG_FOREGROUND_COLOR) {
        text->color = style->foreground_color;
    }
    if (style->flags & UI_STYLE_FLAG_BACKGROUND_COLOR) {
        text->background_color = style->background_color;
    }
}

static bool ui_text_read_codepoint(const char *str, size_t len, size_t offset, uint32_t *out,
                                   size_t *advance)
{
    if (!str || offset >= len || !advance) {
        return false;
    }
    const unsigned char first = (unsigned char)str[offset];
    uint32_t cp = first;
    size_t adv = 1;
    if (first < 0x80) {
    } else if ((first & 0xE0) == 0xC0) {
        cp = first & 0x1F;
        adv = 2;
    } else if ((first & 0xF0) == 0xE0) {
        cp = first & 0x0F;
        adv = 3;
    } else if ((first & 0xF8) == 0xF0) {
        cp = first & 0x07;
        adv = 4;
    } else {
        return false;
    }
    if (offset + adv > len) {
        return false;
    }
    for (size_t i = 1; i < adv; ++i) {
        unsigned char c = (unsigned char)str[offset + i];
        if ((c & 0xC0) != 0x80) {
            return false;
        }
        cp = (cp << 6) | (c & 0x3F);
    }
    *advance = adv;
    if (out) {
        *out = cp;
    }
    return true;
}

static char *ui_text_strdup(const char *src)
{
    if (!src) {
        return NULL;
    }
    size_t len = strlen(src) + 1;
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, src, len);
    }
    return copy;
}

static size_t ui_text_line_break(const ui_text_t *text, const char *line, size_t total_len,
                                 size_t cursor, int max_width, size_t *out_newline)
{
    if (!text || cursor >= total_len) {
        return total_len;
    }
    const bareui_font_t *font = text->font ? text->font : bareui_font_default();
    size_t pos = cursor;
    size_t last_space = cursor;
    int width = 0;
    *out_newline = cursor;

    while (pos < total_len) {
        if (line[pos] == '\n') {
            *out_newline = pos;
            return pos;
        }
        uint32_t cp;
        size_t adv;
        if (!ui_text_read_codepoint(line, total_len, pos, &cp, &adv)) {
            break;
        }
        bareui_font_glyph_t glyph;
        if (bareui_font_lookup(font, cp, &glyph)) {
            width += glyph.spacing;
        }
        if (max_width > 0 && width > max_width) {
            if (last_space > cursor) {
                *out_newline = last_space;
                return last_space;
            }
            *out_newline = pos;
            return pos;
        }
        if (cp == ' ') {
            last_space = pos;
        }
        pos += adv;
    }
    return pos;
}

static int ui_text_measure_line(const ui_text_t *text, const char *line, size_t len)
{
    const bareui_font_t *font = text->font ? text->font : bareui_font_default();
    size_t offset = 0;
    int width = 0;
    while (offset < len) {
        uint32_t cp;
        size_t adv;
        if (!ui_text_read_codepoint(line, len, offset, &cp, &adv)) {
            break;
        }
        bareui_font_glyph_t glyph;
        if (bareui_font_lookup(font, cp, &glyph)) {
            width += glyph.spacing;
        }
        offset += adv;
    }
    return width;
}

static int ui_text_count_lines(const ui_text_t *text, const char *start, size_t total_len,
                               int max_lines, int width)
{
    if (!text || !start) {
        return 0;
    }
    if (width <= 0) {
        width = 1;
    }
    size_t cursor = 0;
    int lines = 0;
    while (cursor < total_len && lines < max_lines) {
        size_t newline_pos = cursor;
        size_t end = newline_pos;
        if (!text->no_wrap) {
            end = ui_text_line_break(text, start, total_len, cursor, width, &newline_pos);
        } else {
            while (end < total_len && start[end] != '\n') {
                ++end;
            }
        }
        size_t line_len = end - cursor;
        if (line_len == 0 && start[cursor] == '\n') {
            ++lines;
            cursor = end + 1;
            continue;
        }
        ++lines;
        if (!text->no_wrap && end < total_len && start[end] == '\n') {
            cursor = end + 1;
        } else {
            cursor = end;
        }
        if (text->no_wrap) {
            break;
        }
    }
    return lines;
}

static void ui_text_draw_line(ui_context_t *ctx, const ui_text_t *text, const char *line,
                              size_t len, int x, int y)
{
    char *buffer = malloc(len + 1);
    if (!buffer) {
        return;
    }
    memcpy(buffer, line, len);
    buffer[len] = '\0';
    const bareui_font_t *font = text->font ? text->font : bareui_font_default();
    const bareui_font_t *prev = ui_context_font(ctx);
    ui_context_set_font(ctx, font);
    ui_context_draw_text(ctx, x, y, buffer, text->color);
    ui_context_set_font(ctx, prev);
    free(buffer);
}

static bool ui_text_render(ui_context_t *ctx, ui_widget_t *widget, const ui_rect_t *bounds)
{
    ui_text_t *text = (ui_text_t *)widget;
    if (!text || !bounds) {
        return false;
    }
    ui_context_fill_rect(ctx, bounds->x, bounds->y, bounds->width, bounds->height,
                         text->background_color);
    if (!text->value || *text->value == '\0') {
        return true;
    }
    const char *start = text->value;
    size_t total_len = strlen(start);
    size_t cursor = 0;
    int drawn = 0;
    int line_height = (text->font ? text->font->height : BAREUI_FONT_HEIGHT) + text->line_spacing;
    int max_lines = text->max_lines > 0 ? text->max_lines : INT_MAX;
    int line_count = ui_text_count_lines(text, start, total_len, max_lines, bounds->width);
    int content_height = line_count * line_height;
    int vertical_offset = 0;
    if (content_height < bounds->height) {
        vertical_offset = (bounds->height - content_height) / 2;
    }
    while (cursor < total_len && drawn < max_lines) {
        size_t newline_pos = cursor;
        size_t end = newline_pos;
        if (!text->no_wrap) {
            end = ui_text_line_break(text, start, total_len, cursor, bounds->width, &newline_pos);
        } else {
            while (end < total_len && start[end] != '\n') {
                ++end;
            }
        }
        size_t line_len = end - cursor;
        if (line_len == 0 && start[cursor] == '\n') {
            cursor = end + 1;
            ++drawn;
            continue;
        }
        int line_width = ui_text_measure_line(text, start + cursor, line_len);
        int x_point = bounds->x;
        if (text->align == UI_TEXT_ALIGN_CENTER) {
            x_point = bounds->x + (bounds->width - line_width) / 2;
        } else if (text->align == UI_TEXT_ALIGN_RIGHT) {
            x_point = bounds->x + bounds->width - line_width;
        }
        if (text->rtl) {
            x_point = bounds->x + bounds->width - (x_point - bounds->x) - line_width;
        }
        int y_point = bounds->y + vertical_offset + drawn * line_height;
        if (line_width > bounds->width && text->overflow == UI_TEXT_OVERFLOW_FADE) {
            /* fade effect not implemented yet; fallback to clip */
        }
        ui_text_draw_line(ctx, text, start + cursor, line_len, x_point, y_point);
        ++drawn;
        if (!text->no_wrap && end < total_len && start[end] == '\n') {
            cursor = end + 1;
        } else {
            cursor = end;
        }
        if (text->no_wrap) {
            break;
        }
    }
    return true;
}

static const ui_widget_ops_t ui_text_ops = {
    .render = ui_text_render,
    .handle_event = NULL,
    .destroy = NULL,
    .style_changed = ui_text_apply_style
};

static void ui_text_set_value_internal(ui_text_t *text, const char *value)
{
    free(text->value);
    text->value = ui_text_strdup(value);
}

ui_text_t *ui_text_create(void)
{
    ui_text_t *text = calloc(1, sizeof(*text));
    if (!text) {
        return NULL;
    }
    ui_text_init(text);
    return text;
}

void ui_text_destroy(ui_text_t *text)
{
    if (!text) {
        return;
    }
    free(text->value);
    free(text);
}

void ui_text_init(ui_text_t *text)
{
    if (!text) {
        return;
    }
    ui_widget_init(&text->base, &ui_text_ops);
    text->value = NULL;
    text->color = ui_color_from_hex(0xFFFFFF);
    text->background_color = 0;
    text->font = bareui_font_default();
    text->align = UI_TEXT_ALIGN_LEFT;
    text->overflow = UI_TEXT_OVERFLOW_CLIP;
    text->no_wrap = false;
    text->rtl = false;
    text->italic = false;
    text->max_lines = 0;
    text->line_spacing = 0;
    ui_style_t default_style;
    ui_style_init(&default_style);
    default_style.foreground_color = text->color;
    default_style.flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    default_style.background_color = text->background_color;
    default_style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(&text->base, &default_style);
}

void ui_text_set_value(ui_text_t *text, const char *value)
{
    if (!text) {
        return;
    }
    ui_text_set_value_internal(text, value);
}

const char *ui_text_value(const ui_text_t *text)
{
    return text ? text->value : NULL;
}

void ui_text_set_color(ui_text_t *text, ui_color_t color)
{
    if (!text) {
        return;
    }
    ui_style_t *style = &text->base.style;
    style->foreground_color = color;
    style->flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    if (text->base.ops && text->base.ops->style_changed) {
        text->base.ops->style_changed(&text->base, style);
    }
}

void ui_text_set_background_color(ui_text_t *text, ui_color_t color)
{
    if (!text) {
        return;
    }
    ui_style_t *style = &text->base.style;
    style->background_color = color;
    style->flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    if (text->base.ops && text->base.ops->style_changed) {
        text->base.ops->style_changed(&text->base, style);
    }
}

void ui_text_set_font(ui_text_t *text, const bareui_font_t *font)
{
    if (text) {
        text->font = font ? font : bareui_font_default();
    }
}

const bareui_font_t *ui_text_font(const ui_text_t *text)
{
    return text ? text->font : NULL;
}

void ui_text_set_align(ui_text_t *text, ui_text_align_t align)
{
    if (text) {
        text->align = align;
    }
}

ui_text_align_t ui_text_align(const ui_text_t *text)
{
    return text ? text->align : UI_TEXT_ALIGN_LEFT;
}

void ui_text_set_overflow(ui_text_t *text, ui_text_overflow_t overflow)
{
    if (text) {
        text->overflow = overflow;
    }
}

ui_text_overflow_t ui_text_overflow(const ui_text_t *text)
{
    return text ? text->overflow : UI_TEXT_OVERFLOW_CLIP;
}

void ui_text_set_no_wrap(ui_text_t *text, bool no_wrap)
{
    if (text) {
        text->no_wrap = no_wrap;
    }
}

bool ui_text_no_wrap(const ui_text_t *text)
{
    return text ? text->no_wrap : false;
}

void ui_text_set_max_lines(ui_text_t *text, int max_lines)
{
    if (text) {
        text->max_lines = max_lines;
    }
}

int ui_text_max_lines(const ui_text_t *text)
{
    return text ? text->max_lines : 0;
}

void ui_text_set_line_spacing(ui_text_t *text, int spacing)
{
    if (text && spacing >= 0) {
        text->line_spacing = spacing;
    }
}

int ui_text_line_spacing(const ui_text_t *text)
{
    return text ? text->line_spacing : 0;
}

void ui_text_set_rtl(ui_text_t *text, bool rtl)
{
    if (text) {
        text->rtl = rtl;
    }
}

bool ui_text_rtl(const ui_text_t *text)
{
    return text ? text->rtl : false;
}

void ui_text_set_italic(ui_text_t *text, bool italic)
{
    if (text) {
        text->italic = italic;
    }
}

bool ui_text_italic(const ui_text_t *text)
{
    return text ? text->italic : false;
}

const ui_widget_t *ui_text_widget(const ui_text_t *text)
{
    return text ? &text->base : NULL;
}

ui_widget_t *ui_text_widget_mutable(ui_text_t *text)
{
    return text ? &text->base : NULL;
}
