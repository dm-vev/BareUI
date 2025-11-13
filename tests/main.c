#include "ui_button.h"
#include "ui_checkbox.h"
#include "ui_column.h"
#include "ui_progressring.h"
#include "ui_progressbar.h"
#include "ui_row.h"
#include "ui_scene.h"
#include "ui_slider.h"
#include "ui_switch.h"
#include "ui_text.h"
#include "ui_hal_test.h"
#include "ui_shadow.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    ui_scene_t *scene;
    ui_column_t *root;
    ui_slider_t *slider;
    ui_switch_t *mode_switch;
    ui_checkbox_t *ring_checkbox;
    ui_progressbar_t *progress_bar;
    ui_progressring_t *progress_ring;
    ui_text_t *status;
    ui_text_t *detail;
    double progress_value;
    int theme_index;
} app_state_t;

static ui_text_t *create_text_line(const char *value, ui_color_t fg, ui_color_t bg, int height)
{
    ui_text_t *text = ui_text_create();
    if (!text) {
        return NULL;
    }
    ui_text_set_value(text, value);
    ui_text_set_font(text, bareui_font_default());
    ui_style_t style;
    ui_style_init(&style);
    style.foreground_color = fg;
    style.background_color = bg;
    style.flags = UI_STYLE_FLAG_FOREGROUND_COLOR | UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_text_widget_mutable(text), &style);
    ui_widget_set_bounds(ui_text_widget_mutable(text), 0, 0, UI_FRAMEBUFFER_WIDTH, height);
    return text;
}

static void apply_column_background(ui_column_t *column, ui_color_t background)
{
    if (!column) {
        return;
    }
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_column_widget_mutable(column), &style);
}

static void style_panel_row(ui_row_t *row, ui_color_t background, int height)
{
    if (!row) {
        return;
    }
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.border_color = ui_color_from_hex(0x2B3145);
    style.border_width = 1;
    style.border_sides = UI_BORDER_LEFT | UI_BORDER_RIGHT | UI_BORDER_TOP | UI_BORDER_BOTTOM;
    style.box_shadow.enabled = true;
    style.box_shadow.color = ui_color_from_hex(0x05090F);
    style.box_shadow.blur_radius = 6;
    style.box_shadow.offset_y = 2;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_BORDER_COLOR |
                  UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BORDER_SIDES | UI_STYLE_FLAG_BOX_SHADOW;
    ui_widget_set_style(ui_row_widget_mutable(row), &style);
    ui_widget_set_bounds(ui_row_widget_mutable(row), 0, 0, UI_FRAMEBUFFER_WIDTH, height);
}

static ui_button_t *create_button(const char *label, ui_color_t background, ui_color_t accent)
{
    ui_button_t *button = ui_button_create();
    if (!button) {
        return NULL;
    }
    ui_button_set_text(button, label);
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.foreground_color = ui_color_from_hex(0xF6F6FF);
    style.accent_color = accent;
    style.border_color = ui_color_from_hex(0x131828);
    style.border_width = 1;
    style.border_sides = UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM;
    style.box_shadow.enabled = true;
    style.box_shadow.color = ui_color_from_hex(0x04060D);
    style.box_shadow.blur_radius = 4;
    style.box_shadow.offset_y = 1;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_FOREGROUND_COLOR |
                  UI_STYLE_FLAG_ACCENT_COLOR | UI_STYLE_FLAG_BORDER_COLOR |
                  UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BORDER_SIDES | UI_STYLE_FLAG_BOX_SHADOW;
    ui_widget_set_style(ui_button_widget_mutable(button), &style);
    ui_widget_set_bounds(ui_button_widget_mutable(button), 0, 0, 120, 40);
    return button;
}

static void update_status(app_state_t *app, const char *message)
{
    if (!app || !app->status) {
        return;
    }
    ui_text_set_value(app->status, message);
}

static void refresh_progress(app_state_t *app)
{
    if (!app) {
        return;
    }
    double percent = app->progress_value * 100.0;
    if (app->progress_bar) {
        ui_progressbar_set_value(app->progress_bar, app->progress_value);
        char label[64];
        snprintf(label, sizeof(label), "%.0f%% готово", percent);
        ui_progressbar_set_semantics_value(app->progress_bar, label);
    }
    if (app->progress_ring) {
        ui_progressring_set_value(app->progress_ring, app->progress_value);
        char label[64];
        snprintf(label, sizeof(label), "%.0f%%", percent);
        ui_progressring_set_semantics_value(app->progress_ring, label);
    }
    if (app->detail) {
        char detail[64];
        snprintf(detail, sizeof(detail), "Готовность: %.0f%%", percent);
        ui_text_set_value(app->detail, detail);
    }
}

static void apply_progress_mode(app_state_t *app, bool turbo)
{
    if (!app) {
        return;
    }
    ui_style_t bar_style;
    ui_style_init(&bar_style);
    bar_style.background_color = turbo ? ui_color_from_hex(0x14202C) : ui_color_from_hex(0x1B2137);
    bar_style.foreground_color = turbo ? ui_color_from_hex(0x8DFCE5) : ui_color_from_hex(0x63A3FF);
    bar_style.accent_color = bar_style.foreground_color;
    bar_style.border_color = ui_color_from_hex(0x2B3145);
    bar_style.border_width = 1;
    bar_style.border_sides = UI_BORDER_LEFT | UI_BORDER_RIGHT | UI_BORDER_TOP | UI_BORDER_BOTTOM;
    bar_style.box_shadow.enabled = true;
    bar_style.box_shadow.color = ui_color_from_hex(0x05090F);
    bar_style.box_shadow.blur_radius = 6;
    bar_style.box_shadow.offset_y = 2;
    bar_style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_FOREGROUND_COLOR |
                      UI_STYLE_FLAG_ACCENT_COLOR | UI_STYLE_FLAG_BORDER_COLOR |
                      UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BORDER_SIDES | UI_STYLE_FLAG_BOX_SHADOW;
    if (app->progress_bar) {
        ui_widget_set_style(ui_progressbar_widget_mutable(app->progress_bar), &bar_style);
    }
    if (app->progress_ring) {
        ui_progressring_set_track_color(app->progress_ring, turbo ? ui_color_from_hex(0x0D1726)
                                                                  : ui_color_from_hex(0x101827));
        ui_progressring_set_progress_color(app->progress_ring, turbo ? ui_color_from_hex(0x9EF3D4)
                                                                     : ui_color_from_hex(0x63A3FF));
    }
}

static void on_increment_click(ui_button_t *button, void *user_data)
{
    (void)button;
    app_state_t *app = user_data;
    if (!app) {
        return;
    }
    app->progress_value += 0.22;
    if (app->progress_value > 1.0) {
        app->progress_value = 1.0;
    }
    if (app->slider) {
        ui_slider_set_value(app->slider, app->progress_value * 100.0);
    }
    refresh_progress(app);
    update_status(app, "Boost режим увеличил индикаторы");
}

static void on_reset_click(ui_button_t *button, void *user_data)
{
    (void)button;
    app_state_t *app = user_data;
    if (!app) {
        return;
    }
    app->progress_value = 0.34;
    if (app->slider) {
        ui_slider_set_value(app->slider, app->progress_value * 100.0);
    }
    refresh_progress(app);
    update_status(app, "Сброс значений до базовых");
}

static void on_theme_click(ui_button_t *button, void *user_data)
{
    (void)button;
    app_state_t *app = user_data;
    if (!app || !app->root) {
        return;
    }
    static const uint32_t palettes[] = {
        0x0F1420,
        0x1A1E2C,
        0x111928
    };
    app->theme_index = (app->theme_index + 1) % (int)(sizeof(palettes) / sizeof(palettes[0]));
    ui_color_t color = ui_color_from_hex(palettes[app->theme_index]);
    apply_column_background(app->root, color);
    update_status(app, "Тема сцены обновлена");
}

static void on_switch_change(ui_switch_t *sw, bool value, void *user_data)
{
    (void)sw;
    app_state_t *app = user_data;
    if (!app) {
        return;
    }
    apply_progress_mode(app, value);
    if (value) {
        update_status(app, "Performance режим активен");
    } else {
        update_status(app, "Balanced режим включен");
    }
}

static void on_checkbox_change(ui_checkbox_t *checkbox, ui_checkbox_state_t state, void *user_data)
{
    (void)checkbox;
    app_state_t *app = user_data;
    if (!app) {
        return;
    }
    bool visible = state != UI_CHECKBOX_STATE_UNCHECKED;
    if (app->progress_ring) {
        ui_widget_set_visible(ui_progressring_widget_mutable(app->progress_ring), visible);
    }
    update_status(app, visible ? "Кольцо отображается" : "Кольцо скрыто");
}

static void on_slider_change(ui_slider_t *slider, double value, void *user_data)
{
    (void)slider;
    app_state_t *app = user_data;
    if (!app) {
        return;
    }
    app->progress_value = value / 100.0;
    refresh_progress(app);
    update_status(app, "Слайдер синхронизирует индикаторы");
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

    app_state_t app = {
        .scene = scene,
        .root = NULL,
        .slider = NULL,
        .mode_switch = NULL,
        .ring_checkbox = NULL,
        .progress_bar = NULL,
        .progress_ring = NULL,
        .status = NULL,
        .detail = NULL,
        .progress_value = 0.34,
        .theme_index = 0
    };

    ui_column_t *root = NULL;
    ui_row_t *control_row = NULL;
    ui_row_t *ring_row = NULL;
    ui_row_t *toggles = NULL;
    ui_row_t *status_row = NULL;
    ui_text_t *title = NULL;
    ui_text_t *subtitle = NULL;
    ui_text_t *ring_hint = NULL;
    ui_text_t *status_line = NULL;
    ui_text_t *detail_line = NULL;
    ui_button_t *boost_btn = NULL;
    ui_button_t *reset_btn = NULL;
    ui_button_t *theme_btn = NULL;
    ui_progressring_t *ring = NULL;
    ui_slider_t *slider = NULL;
    ui_progressbar_t *progress = NULL;
    ui_switch_t *mode = NULL;
    ui_checkbox_t *ring_toggle = NULL;
    int exit_code = 1;

    root = ui_column_create();
    if (!root) {
        goto cleanup;
    }
    app.root = root;
    ui_column_set_spacing(root, 12);
    apply_column_background(root, ui_color_from_hex(0x0F1420));
    ui_widget_set_bounds(ui_column_widget_mutable(root), 0, 0, UI_FRAMEBUFFER_WIDTH, UI_FRAMEBUFFER_HEIGHT);

    title = create_text_line("BareUI Single Scene Lab", ui_color_from_hex(0xFAFAFF),
                             ui_color_from_hex(0x0F1420), 36);
    if (!title) {
        goto cleanup;
    }
    subtitle = create_text_line(
        "Все виджеты на одной сцене и взаимодействуют друг с другом", ui_color_from_hex(0x91C7FF),
        ui_color_from_hex(0x0F1420), 28);
    if (!subtitle) {
        goto cleanup;
    }
    ui_column_add_control(root, ui_text_widget_mutable(title), false, "title");
    ui_column_add_control(root, ui_text_widget_mutable(subtitle), false, "subtitle");

    control_row = ui_row_create();
    if (!control_row) {
        goto cleanup;
    }
    ui_row_set_spacing(control_row, 8);
    style_panel_row(control_row, ui_color_from_hex(0x12182B), 56);
    boost_btn = create_button("Boost", ui_color_from_hex(0x1B3A73), ui_color_from_hex(0x90D2FF));
    if (!boost_btn) {
        goto cleanup;
    }
    reset_btn = create_button("Reset", ui_color_from_hex(0x2A2F3D), ui_color_from_hex(0xFF9A7B));
    if (!reset_btn) {
        goto cleanup;
    }
    theme_btn = create_button("Theme", ui_color_from_hex(0x3B2C6F), ui_color_from_hex(0xC0A0FF));
    if (!theme_btn) {
        goto cleanup;
    }
    ui_button_set_on_click(boost_btn, on_increment_click, &app);
    ui_button_set_on_click(reset_btn, on_reset_click, &app);
    ui_button_set_on_click(theme_btn, on_theme_click, &app);
    ui_row_add_control(control_row, ui_button_widget_mutable(boost_btn), true, "boost");
    ui_row_add_control(control_row, ui_button_widget_mutable(reset_btn), true, "reset");
    ui_row_add_control(control_row, ui_button_widget_mutable(theme_btn), true, "theme");
    ui_column_add_control(root, ui_row_widget_mutable(control_row), false, "controls");

    ring_row = ui_row_create();
    if (!ring_row) {
        goto cleanup;
    }
    ui_row_set_spacing(ring_row, 12);
    style_panel_row(ring_row, ui_color_from_hex(0x12182B), 92);
    ring = ui_progressring_create();
    if (!ring) {
        goto cleanup;
    }
    ui_widget_set_bounds(ui_progressring_widget_mutable(ring), 0, 0, 68, 68);
    ui_progressring_set_value(ring, app.progress_value);
    ui_progressring_set_progress_color(ring, ui_color_from_hex(0x6FFDDC));
    ui_progressring_set_track_color(ring, ui_color_from_hex(0x111828));
    ui_progressring_set_tooltip(ring, "Синхронизированный индикатор");
    ui_row_add_control(ring_row, ui_progressring_widget_mutable(ring), false, "ring");
    ring_hint = create_text_line("Кольцо отражает слайдер", ui_color_from_hex(0xCED8FF),
                                 ui_color_from_hex(0x12182B), 64);
    if (!ring_hint) {
        goto cleanup;
    }
    ui_row_add_control(ring_row, ui_text_widget_mutable(ring_hint), true, "ring_hint");
    ui_column_add_control(root, ui_row_widget_mutable(ring_row), false, "ring_row");
    app.progress_ring = ring;

    slider = ui_slider_create();
    if (!slider) {
        goto cleanup;
    }
    app.slider = slider;
    ui_slider_set_min(slider, 0);
    ui_slider_set_max(slider, 100);
    ui_slider_set_divisions(slider, 20);
    ui_slider_set_round(slider, 0);
    ui_slider_set_label(slider, "{value}%");
    ui_slider_set_active_color(slider, ui_color_from_hex(0x6FFDDC));
    ui_slider_set_inactive_color(slider, ui_color_from_hex(0x101523));
    ui_slider_set_thumb_color(slider, ui_color_from_hex(0xF6F6FF));
    ui_slider_set_overlay_color(slider, ui_color_from_hex(0x6FFDDC));
    ui_slider_set_on_change(slider, on_slider_change, &app);
    ui_slider_set_value(slider, app.progress_value * 100.0);
    ui_style_t slider_style;
    ui_style_init(&slider_style);
    slider_style.background_color = ui_color_from_hex(0x12182B);
    slider_style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_slider_widget_mutable(slider), &slider_style);
    ui_widget_set_bounds(ui_slider_widget_mutable(slider), 0, 0, UI_FRAMEBUFFER_WIDTH, 52);
    ui_column_add_control(root, ui_slider_widget_mutable(slider), false, "slider");

    progress = ui_progressbar_create();
    if (!progress) {
        goto cleanup;
    }
    app.progress_bar = progress;
    ui_widget_set_bounds(ui_progressbar_widget_mutable(progress), 0, 0, UI_FRAMEBUFFER_WIDTH, 30);
    ui_progressbar_set_bar_height(progress, 8);
    ui_progressbar_set_border_radius(progress, ui_border_radius_all(10));
    ui_progressbar_set_tooltip(progress, "Линейный индикатор");
    ui_column_add_control(root, ui_progressbar_widget_mutable(progress), false, "bar");

    toggles = ui_row_create();
    if (!toggles) {
        goto cleanup;
    }
    ui_row_set_spacing(toggles, 12);
    style_panel_row(toggles, ui_color_from_hex(0x12182B), 52);
    mode = ui_switch_create();
    if (mode) {
        ui_switch_set_label(mode, "Performance");
        ui_switch_set_value(mode, true);
        ui_switch_set_on_change(mode, on_switch_change, &app);
        ui_row_add_control(toggles, ui_switch_widget_mutable(mode), true, "mode");
        app.mode_switch = mode;
    }
    ring_toggle = ui_checkbox_create();
    if (ring_toggle) {
        ui_checkbox_set_label(ring_toggle, "Show gauge");
        ui_checkbox_set_state(ring_toggle, UI_CHECKBOX_STATE_CHECKED);
        ui_checkbox_set_on_change(ring_toggle, on_checkbox_change, &app);
        ui_row_add_control(toggles, ui_checkbox_widget_mutable(ring_toggle), true, "ring_toggle");
        app.ring_checkbox = ring_toggle;
    }
    ui_column_add_control(root, ui_row_widget_mutable(toggles), false, "toggles");

    status_row = ui_row_create();
    if (!status_row) {
        goto cleanup;
    }
    ui_row_set_spacing(status_row, 12);
    style_panel_row(status_row, ui_color_from_hex(0x12182B), 48);
    status_line = create_text_line("Готов к взаимодействию", ui_color_from_hex(0xF7F7FF),
                                   ui_color_from_hex(0x12182B), 44);
    if (!status_line) {
        goto cleanup;
    }
    detail_line = create_text_line("Готовность: 34%", ui_color_from_hex(0x9DEFFF),
                                   ui_color_from_hex(0x12182B), 44);
    if (!detail_line) {
        goto cleanup;
    }
    app.status = status_line;
    app.detail = detail_line;
    ui_row_add_control(status_row, ui_text_widget_mutable(status_line), true, "status_text");
    ui_row_add_control(status_row, ui_text_widget_mutable(detail_line), true, "detail_text");
    ui_column_add_control(root, ui_row_widget_mutable(status_row), false, "status_row");

    apply_progress_mode(&app, true);
    refresh_progress(&app);

    ui_scene_set_root(scene, ui_column_widget_mutable(root));
    ui_scene_run(scene);
    exit_code = 0;

cleanup:
    ui_row_destroy(status_row);
    ui_row_destroy(toggles);
    ui_checkbox_destroy(ring_toggle);
    ui_switch_destroy(mode);
    ui_progressbar_destroy(progress);
    ui_slider_destroy(slider);
    ui_text_destroy(ring_hint);
    ui_progressring_destroy(ring);
    ui_row_destroy(ring_row);
    ui_row_destroy(control_row);
    ui_column_destroy(root);
    ui_button_destroy(boost_btn);
    ui_button_destroy(reset_btn);
    ui_button_destroy(theme_btn);
    ui_text_destroy(title);
    ui_text_destroy(subtitle);
    ui_text_destroy(status_line);
    ui_text_destroy(detail_line);
    if (scene) {
        ui_scene_destroy(scene);
    }
    return exit_code;
}
