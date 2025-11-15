#include "ui_border_radius.h"
#include "ui_button.h"
#include "ui_column.h"
#include "ui_font.h"
#include "ui_hal_test.h"
#include "ui_progressbar.h"
#include "ui_progressring.h"
#include "ui_slider.h"
#include "ui_switch.h"
#include "ui_tab.h"
#include "ui_text.h"
#include "ui_widget.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define TAB_DEMO_TEXT_MAX 16
#define TAB_DEMO_BUTTON_MAX 3

typedef struct {
    ui_color_t seashell;
    ui_color_t champagne;
    ui_color_t rosy_brown;
    ui_color_t burnt_sienna;
    ui_color_t redwood;
    ui_color_t dark_text;
} classic_noir_palette_t;

#define CLASSIC_NOIR_SEASHELL 0xEEE2DF
#define CLASSIC_NOIR_CHAMPAGNE 0xEED7C5
#define CLASSIC_NOIR_ROSY 0xC89F9C
#define CLASSIC_NOIR_BURNT 0xC97C5D
#define CLASSIC_NOIR_REDWOOD 0xB36A5E
#define CLASSIC_NOIR_DARK 0x1A1A1A

typedef struct {
    ui_text_t *value_label;
} slider_value_ctx_t;

static bool track_text(ui_text_t **storage, size_t *count, ui_text_t *text)
{
    if (!storage || !count || !text || *count >= TAB_DEMO_TEXT_MAX) {
        return false;
    }
    storage[(*count)++] = text;
    return true;
}

static bool track_button(ui_button_t **storage, size_t *count, ui_button_t *button)
{
    if (!storage || !count || !button || *count >= TAB_DEMO_BUTTON_MAX) {
        return false;
    }
    storage[(*count)++] = button;
    return true;
}

static void update_slider_value(ui_slider_t *slider, double value, void *user_data)
{
    (void)slider;
    slider_value_ctx_t *ctx = user_data;
    if (!ctx || !ctx->value_label) {
        return;
    }
    char label[32];
    snprintf(label, sizeof(label), "%.0f%%", value * 100.0);
    ui_text_set_value(ctx->value_label, label);
}

static void style_tab_column(ui_column_t *column, ui_color_t background, ui_color_t border)
{
    if (!column) {
        return;
    }
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    style.border_color = border;
    style.border_width = 1;
    style.border_sides = UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM;
    style.flags |= UI_STYLE_FLAG_BORDER_COLOR | UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BORDER_SIDES;
    style.padding_left = 12;
    style.padding_right = 12;
    style.padding_top = 12;
    style.padding_bottom = 12;
    ui_widget_set_style(ui_column_widget_mutable(column), &style);
}

int main(void)
{
    const ui_hal_ops_t *hal = ui_hal_test_sdl_ops();
    if (!hal) {
        return 1;
    }

    ui_scene_t *scene = ui_scene_create(hal);
    if (!scene) {
        return 1;
    }

    classic_noir_palette_t palette = {
        .seashell = ui_color_from_hex(CLASSIC_NOIR_SEASHELL),
        .champagne = ui_color_from_hex(CLASSIC_NOIR_CHAMPAGNE),
        .rosy_brown = ui_color_from_hex(CLASSIC_NOIR_ROSY),
        .burnt_sienna = ui_color_from_hex(CLASSIC_NOIR_BURNT),
        .redwood = ui_color_from_hex(CLASSIC_NOIR_REDWOOD),
        .dark_text = ui_color_from_hex(CLASSIC_NOIR_DARK),
    };

    ui_column_t *root = ui_column_create();
    if (!root) {
        ui_scene_destroy(scene);
        return 1;
    }

    const int root_spacing = 12;
    ui_column_set_spacing(root, root_spacing);
    ui_column_set_alignment(root, UI_MAIN_AXIS_START);
    ui_column_set_horizontal_alignment(root, UI_CROSS_AXIS_CENTER);

    ui_style_t root_style;
    ui_style_init(&root_style);
    root_style.background_color = palette.seashell;
    root_style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    root_style.padding_top = 16;
    root_style.padding_bottom = 16;
    root_style.padding_left = 16;
    root_style.padding_right = 16;
    ui_widget_set_style(ui_column_widget_mutable(root), &root_style);
    ui_widget_set_bounds(ui_column_widget_mutable(root), 0, 0, UI_FRAMEBUFFER_WIDTH, UI_FRAMEBUFFER_HEIGHT);

    const bareui_font_t *font = bareui_font_default();
    const int title_height = font->height + 6;
    const int subtitle_height = font->height + 4;
    const int content_width = UI_FRAMEBUFFER_WIDTH - root_style.padding_left - root_style.padding_right;

    ui_text_t *text_widgets[TAB_DEMO_TEXT_MAX] = {0};
    size_t text_count = 0;
    ui_button_t *buttons[TAB_DEMO_BUTTON_MAX] = {0};
    size_t button_count = 0;
    ui_switch_t *night_switch = NULL;
    ui_progressring_t *ambient_ring = NULL;
    ui_slider_t *tone_slider = NULL;
    ui_progressbar_t *progress_bar = NULL;
    ui_tabs_t *tabs = NULL;
    int exit_code = 1;

    ui_text_t *title = ui_text_create();
    if (!title) {
        goto cleanup;
    }
    ui_text_set_value(title, "Classic Noir Tabs");
    ui_text_set_color(title, palette.redwood);
    ui_text_set_align(title, UI_TEXT_ALIGN_LEFT);
    ui_text_set_no_wrap(title, true);
    ui_text_set_background_color(title, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(title), 0, 0, content_width, title_height);
    if (!track_text(text_widgets, &text_count, title)) {
        goto cleanup;
    }
    if (!ui_column_add_control(root, ui_text_widget_mutable(title), false, "headline")) {
        goto cleanup;
    }

    ui_text_t *subtitle = ui_text_create();
    if (!subtitle) {
        goto cleanup;
    }
    ui_text_set_value(subtitle, "Navigate tactile controls that glow with Classic Noir charcoal and ember tones.");
    ui_text_set_color(subtitle, palette.dark_text);
    ui_text_set_align(subtitle, UI_TEXT_ALIGN_LEFT);
    ui_text_set_no_wrap(subtitle, false);
    ui_text_set_background_color(subtitle, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(subtitle), 0, 0, content_width, subtitle_height);
    if (!track_text(text_widgets, &text_count, subtitle)) {
        goto cleanup;
    }
    if (!ui_column_add_control(root, ui_text_widget_mutable(subtitle), false, "subtitle")) {
        goto cleanup;
    }

    tabs = ui_tabs_create();
    if (!tabs) {
        goto cleanup;
    }
    ui_style_t tabs_style;
    ui_style_init(&tabs_style);
    tabs_style.background_color = palette.champagne;
    tabs_style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    tabs_style.border_color = palette.redwood;
    tabs_style.border_width = 1;
    tabs_style.border_sides = UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM;
    tabs_style.flags |= UI_STYLE_FLAG_BORDER_COLOR | UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BORDER_SIDES;
    tabs_style.padding_left = 10;
    tabs_style.padding_right = 10;
    tabs_style.padding_top = 8;
    tabs_style.padding_bottom = 8;
    ui_widget_set_style(ui_tabs_widget_mutable(tabs), &tabs_style);

    ui_tabs_set_padding(tabs, (ui_padding_t){6, 6, 12, 12});
    ui_tabs_set_label_padding(tabs, (ui_padding_t){4, 4, 8, 8});
    ui_tabs_set_indicator_color(tabs, palette.redwood);
    ui_tabs_set_indicator_thickness(tabs, 4);
    ui_tabs_set_indicator_padding(tabs, 6);
    ui_tabs_set_indicator_border_radius(tabs, ui_border_radius_all(4));
    ui_tabs_set_indicator_tab_size(tabs, false);
    ui_tabs_set_divider_color(tabs, palette.rosy_brown);
    ui_tabs_set_divider_height(tabs, 1);
    ui_tabs_set_label_color(tabs, palette.dark_text);
    ui_tabs_set_unselected_label_color(tabs, palette.rosy_brown);
    ui_tabs_set_overlay_color(tabs, palette.champagne);
    ui_tabs_set_tab_alignment(tabs, UI_TAB_ALIGNMENT_FILL);
    ui_tabs_set_clip_behavior(tabs, UI_TAB_CLIP_BEHAVIOR_HARD_EDGE);

    const int consumed = root_style.padding_top + root_style.padding_bottom + title_height + subtitle_height + root_spacing * 2;
    int tabs_height = UI_FRAMEBUFFER_HEIGHT - consumed;
    if (tabs_height < 160) {
        tabs_height = 160;
    }
    ui_widget_set_bounds(ui_tabs_widget_mutable(tabs), 0, 0, content_width, tabs_height);
    if (!ui_column_add_control(root, ui_tabs_widget_mutable(tabs), true, "tabs")) {
        goto cleanup;
    }

    ui_column_t *tab_columns[3] = {0};
    for (size_t i = 0; i < 3; ++i) {
        tab_columns[i] = ui_column_create();
        if (!tab_columns[i]) {
            goto cleanup;
        }
        style_tab_column(tab_columns[i], palette.seashell, palette.rosy_brown);
        ui_column_set_spacing(tab_columns[i], 12);
        ui_column_set_alignment(tab_columns[i], UI_MAIN_AXIS_START);
        ui_column_set_horizontal_alignment(tab_columns[i], UI_CROSS_AXIS_STRETCH);
        ui_column_set_scroll_mode(tab_columns[i], UI_SCROLL_MODE_ENABLED);
    }

    const int column_padding_horizontal = 12 + 12;
    const int tabs_padding_horizontal = tabs_style.padding_left + tabs_style.padding_right;
    int tab_content_width = content_width - column_padding_horizontal - tabs_padding_horizontal;
    if (tab_content_width < 0) {
        tab_content_width = 0;
    }

    const int line_height = font->height + 2;
    const int button_height = 32;

    ui_text_t *tab1_header = ui_text_create();
    if (!tab1_header) {
        goto cleanup;
    }
    ui_text_set_value(tab1_header, "Button trio");
    ui_text_set_color(tab1_header, palette.dark_text);
    ui_text_set_align(tab1_header, UI_TEXT_ALIGN_LEFT);
    ui_text_set_background_color(tab1_header, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(tab1_header), 0, 0, tab_content_width, line_height);
    if (!track_text(text_widgets, &text_count, tab1_header)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[0], ui_text_widget_mutable(tab1_header), false, "tab1-label");

    ui_button_t *highlight = ui_button_create();
    if (!highlight) {
        goto cleanup;
    }
    ui_button_set_text(highlight, "Primary action");
    ui_button_set_filled_style(highlight, palette.burnt_sienna, palette.champagne, palette.redwood);
    ui_button_set_text_color(highlight, palette.seashell);
    ui_widget_set_bounds(ui_button_widget_mutable(highlight), 0, 0, tab_content_width, button_height);
    if (!track_button(buttons, &button_count, highlight)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[0], ui_button_widget_mutable(highlight), false, "tab1-primary");

    ui_button_t *tonal = ui_button_create();
    if (!tonal) {
        goto cleanup;
    }
    ui_button_set_text(tonal, "Secondary");
    ui_button_set_background_color(tonal, palette.seashell);
    ui_button_set_hover_color(tonal, palette.champagne);
    ui_button_set_pressed_color(tonal, palette.burnt_sienna);
    ui_button_set_text_color(tonal, palette.dark_text);
    ui_widget_set_bounds(ui_button_widget_mutable(tonal), 0, 0, tab_content_width, button_height);
    if (!track_button(buttons, &button_count, tonal)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[0], ui_button_widget_mutable(tonal), false, "tab1-secondary");

    ui_button_t *ghost = ui_button_create();
    if (!ghost) {
        goto cleanup;
    }
    ui_button_set_text(ghost, "Ghost action");
    ui_button_set_background_color(ghost, 0);
    ui_button_set_hover_color(ghost, palette.rosy_brown);
    ui_button_set_pressed_color(ghost, palette.redwood);
    ui_button_set_text_color(ghost, palette.redwood);
    ui_widget_set_bounds(ui_button_widget_mutable(ghost), 0, 0, tab_content_width, button_height);
    if (!track_button(buttons, &button_count, ghost)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[0], ui_button_widget_mutable(ghost), false, "tab1-ghost");

    ui_text_t *tab2_header = ui_text_create();
    if (!tab2_header) {
        goto cleanup;
    }
    ui_text_set_value(tab2_header, "Ambient switches");
    ui_text_set_color(tab2_header, palette.dark_text);
    ui_text_set_align(tab2_header, UI_TEXT_ALIGN_LEFT);
    ui_text_set_background_color(tab2_header, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(tab2_header), 0, 0, tab_content_width, line_height);
    if (!track_text(text_widgets, &text_count, tab2_header)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[1], ui_text_widget_mutable(tab2_header), false, "tab2-label");

    night_switch = ui_switch_create();
    if (!night_switch) {
        goto cleanup;
    }
    ui_switch_set_label(night_switch, "Night mode");
    ui_switch_set_value(night_switch, true);
    ui_switch_set_active_color(night_switch, palette.redwood);
    ui_switch_set_active_track_color(night_switch, palette.burnt_sienna);
    ui_switch_set_inactive_track_color(night_switch, palette.rosy_brown);
    ui_switch_set_inactive_thumb_color(night_switch, palette.seashell);
    ui_switch_set_track_outline_color(night_switch, palette.dark_text);
    ui_widget_set_bounds(ui_switch_widget_mutable(night_switch), 0, 0, tab_content_width, 32);
    ui_column_add_control(tab_columns[1], ui_switch_widget_mutable(night_switch), false, "night-switch");

    ui_text_t *ring_label = ui_text_create();
    if (!ring_label) {
        goto cleanup;
    }
    ui_text_set_value(ring_label, "Ambient meter · 62% filled");
    ui_text_set_color(ring_label, palette.dark_text);
    ui_text_set_align(ring_label, UI_TEXT_ALIGN_LEFT);
    ui_text_set_background_color(ring_label, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(ring_label), 0, 0, tab_content_width, line_height);
    if (!track_text(text_widgets, &text_count, ring_label)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[1], ui_text_widget_mutable(ring_label), false, "ring-label");

    ambient_ring = ui_progressring_create();
    if (!ambient_ring) {
        goto cleanup;
    }
    ui_progressring_set_value(ambient_ring, 0.62);
    ui_progressring_set_track_color(ambient_ring, palette.champagne);
    ui_progressring_set_progress_color(ambient_ring, palette.redwood);
    ui_progressring_set_stroke_width(ambient_ring, 6.0);
    ui_progressring_set_stroke_cap(ambient_ring, UI_STROKE_CAP_ROUND);
    ui_widget_set_bounds(ui_progressring_widget_mutable(ambient_ring), 0, 0, tab_content_width, 110);
    ui_column_add_control(tab_columns[1], ui_progressring_widget_mutable(ambient_ring), false, "ambient-ring");

    ui_text_t *tab3_header = ui_text_create();
    if (!tab3_header) {
        goto cleanup;
    }
    ui_text_set_value(tab3_header, "Dynamic landscape");
    ui_text_set_color(tab3_header, palette.dark_text);
    ui_text_set_align(tab3_header, UI_TEXT_ALIGN_LEFT);
    ui_text_set_background_color(tab3_header, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(tab3_header), 0, 0, tab_content_width, line_height);
    if (!track_text(text_widgets, &text_count, tab3_header)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[2], ui_text_widget_mutable(tab3_header), false, "tab3-label");

    ui_text_t *slider_label = ui_text_create();
    if (!slider_label) {
        goto cleanup;
    }
    ui_text_set_value(slider_label, "Reverb depth");
    ui_text_set_color(slider_label, palette.dark_text);
    ui_text_set_align(slider_label, UI_TEXT_ALIGN_LEFT);
    ui_text_set_background_color(slider_label, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(slider_label), 0, 0, tab_content_width, line_height);
    if (!track_text(text_widgets, &text_count, slider_label)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[2], ui_text_widget_mutable(slider_label), false, "slider-label");

    tone_slider = ui_slider_create();
    if (!tone_slider) {
        goto cleanup;
    }
    ui_slider_set_value(tone_slider, 0.72);
    ui_slider_set_active_color(tone_slider, palette.burnt_sienna);
    ui_slider_set_inactive_color(tone_slider, palette.rosy_brown);
    ui_slider_set_thumb_color(tone_slider, palette.champagne);
    ui_slider_set_overlay_color(tone_slider, palette.redwood);
    ui_slider_set_label(tone_slider, "Tone");
    ui_widget_set_bounds(ui_slider_widget_mutable(tone_slider), 0, 0, tab_content_width, 28);
    ui_column_add_control(tab_columns[2], ui_slider_widget_mutable(tone_slider), false, "tone-slider");

    ui_text_t *slider_value_text = ui_text_create();
    if (!slider_value_text) {
        goto cleanup;
    }
    ui_text_set_value(slider_value_text, "72%");
    ui_text_set_color(slider_value_text, palette.dark_text);
    ui_text_set_align(slider_value_text, UI_TEXT_ALIGN_RIGHT);
    ui_text_set_background_color(slider_value_text, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(slider_value_text), 0, 0, tab_content_width, line_height);
    if (!track_text(text_widgets, &text_count, slider_value_text)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[2], ui_text_widget_mutable(slider_value_text), false, "slider-value");

    slider_value_ctx_t slider_ctx = {.value_label = slider_value_text};
    ui_slider_set_on_change(tone_slider, update_slider_value, &slider_ctx);
    update_slider_value(tone_slider, ui_slider_value(tone_slider), &slider_ctx);

    ui_text_t *progress_label = ui_text_create();
    if (!progress_label) {
        goto cleanup;
    }
    ui_text_set_value(progress_label, "Progress bar");
    ui_text_set_color(progress_label, palette.dark_text);
    ui_text_set_align(progress_label, UI_TEXT_ALIGN_LEFT);
    ui_text_set_background_color(progress_label, palette.seashell);
    ui_widget_set_bounds(ui_text_widget_mutable(progress_label), 0, 0, tab_content_width, line_height);
    if (!track_text(text_widgets, &text_count, progress_label)) {
        goto cleanup;
    }
    ui_column_add_control(tab_columns[2], ui_text_widget_mutable(progress_label), false, "progress-label");

    progress_bar = ui_progressbar_create();
    if (!progress_bar) {
        goto cleanup;
    }
    ui_progressbar_set_value(progress_bar, 0.58);
    ui_progressbar_set_bar_height(progress_bar, 14);
    ui_progressbar_set_border_radius(progress_bar, ui_border_radius_all(6));
    ui_style_t progress_style;
    ui_style_init(&progress_style);
    progress_style.background_color = palette.rosy_brown;
    progress_style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    progress_style.foreground_color = palette.burnt_sienna;
    progress_style.flags |= UI_STYLE_FLAG_FOREGROUND_COLOR;
    progress_style.accent_color = palette.redwood;
    progress_style.flags |= UI_STYLE_FLAG_ACCENT_COLOR;
    progress_style.border_color = palette.rosy_brown;
    progress_style.border_width = 0;
    progress_style.border_sides = 0;
    ui_widget_set_style(ui_progressbar_widget_mutable(progress_bar), &progress_style);
    ui_widget_set_bounds(ui_progressbar_widget_mutable(progress_bar), 0, 0, tab_content_width, 28);
    ui_column_add_control(tab_columns[2], ui_progressbar_widget_mutable(progress_bar), false, "progress-bar");

    const char *tab_titles[3] = {"Controls", "Indicators", "Sliders"};
    for (size_t i = 0; i < 3; ++i) {
        ui_tab_t *tab = ui_tab_create();
        if (!tab) {
            goto cleanup;
        }
        ui_tab_set_text(tab, tab_titles[i]);
        ui_tab_set_content(tab, ui_column_widget_mutable(tab_columns[i]));
        if (!ui_tabs_add_tab(tabs, tab)) {
            ui_tab_destroy(tab);
            goto cleanup;
        }
    }

    exit_code = 0;
    ui_scene_set_root(scene, ui_column_widget_mutable(root));
    ui_scene_run(scene);

cleanup:
    for (size_t i = 0; i < button_count; ++i) {
        if (buttons[i]) {
            ui_button_destroy(buttons[i]);
        }
    }
    for (size_t i = 0; i < text_count; ++i) {
        if (text_widgets[i]) {
            ui_text_destroy(text_widgets[i]);
        }
    }
    if (tone_slider) {
        ui_slider_destroy(tone_slider);
    }
    if (progress_bar) {
        ui_progressbar_destroy(progress_bar);
    }
    if (ambient_ring) {
        ui_progressring_destroy(ambient_ring);
    }
    if (night_switch) {
        ui_switch_destroy(night_switch);
    }
    if (tabs) {
        ui_tabs_destroy(tabs);
    }
    for (size_t i = 0; i < 3; ++i) {
        if (tab_columns[i]) {
            ui_column_destroy(tab_columns[i]);
        }
    }
    if (root) {
        ui_column_destroy(root);
    }
    if (scene) {
        ui_scene_destroy(scene);
    }
    return exit_code;
}
