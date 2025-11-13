#include "ui_button.h"
#include "ui_column.h"
#include "ui_row.h"
#include "ui_scene.h"
#include "ui_text.h"
#include "ui_hal_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ui_scene_t *scene;
    ui_column_t *column;
    ui_text_t *status;
    ui_text_t *clock;
    ui_text_t *subtitle;
    int color_index;
    double elapsed;
} app_state_t;

static ui_text_t *make_text(const char *value, ui_color_t fg, ui_color_t bg)
{
    ui_text_t *text = ui_text_create();
    if (!text) {
        return NULL;
    }
    ui_style_t style;
    ui_style_init(&style);
    style.foreground_color = fg;
    style.background_color = bg;
    style.flags |= UI_STYLE_FLAG_FOREGROUND_COLOR | UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_text_widget_mutable(text), &style);
    ui_text_set_value(text, value);
    ui_widget_set_bounds(ui_text_widget_mutable(text), 0, 0, UI_FRAMEBUFFER_WIDTH, BAREUI_FONT_HEIGHT + 4);
    return text;
}

static ui_button_t *make_button(const char *label)
{
    ui_button_t *button = ui_button_create();
    if (!button) {
        return NULL;
    }
    ui_button_set_text(button, label);
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = ui_color_from_hex(0x1F1F2B);
    style.foreground_color = ui_color_from_hex(0xF2F2F2);
    style.accent_color = ui_color_from_hex(0x5A9ADB);
    style.border_color = ui_color_from_hex(0x405070);
    style.border_width = 1;
    style.shadow_enabled = true;
    style.shadow_color = ui_color_from_hex(0x000000);
    style.shadow_offset_x = 1;
    style.shadow_offset_y = 1;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_FOREGROUND_COLOR |
                  UI_STYLE_FLAG_ACCENT_COLOR | UI_STYLE_FLAG_BORDER_COLOR |
                  UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_SHADOW;
    ui_widget_set_style(ui_button_widget_mutable(button), &style);
    ui_widget_set_bounds(ui_button_widget_mutable(button), 0, 0, 90, 20);
    return button;
}

static void apply_column_style(ui_column_t *column, ui_color_t background)
{
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_column_widget_mutable(column), &style);
}

static void apply_row_style(ui_row_t *row, ui_color_t background)
{
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_row_widget_mutable(row), &style);
}

static void update_status(app_state_t *app, const char *message)
{
    if (!app || !app->status) {
        return;
    }
    ui_text_set_value(app->status, message);
}

static void on_action_click(ui_button_t *button, void *user_data)
{
    app_state_t *app = user_data;
    if (!app) {
        return;
    }
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Action: %s", ui_button_text(button));
    update_status(app, buffer);
}

static void on_theme_click(ui_button_t *button, void *user_data)
{
    app_state_t *app = user_data;
    if (!app) {
        return;
    }
    (void)button;
    const ui_color_t palette[] = {
        ui_color_from_hex(0x10101F),
        ui_color_from_hex(0x1E2A3C),
        ui_color_from_hex(0x2B2E3E),
        ui_color_from_hex(0x16292D),
    };
    app->color_index = (app->color_index + 1) % (int)(sizeof(palette) / sizeof(palette[0]));
    apply_column_style(app->column, palette[app->color_index]);
    update_status(app, "Theme changed");
}

static bool app_tick(ui_scene_t *scene, double delta)
{
    app_state_t *app = ui_scene_user_data(scene);
    if (!app || !app->clock) {
        return true;
    }
    app->elapsed += delta;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Uptime %.1f s", app->elapsed);
    ui_text_set_value(app->clock, buffer);
    return true;
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
        .status = NULL,
        .clock = NULL,
        .subtitle = NULL,
        .color_index = 0,
        .elapsed = 0.0,
    };

    app.column = ui_column_create();
    if (!app.column) {
        ui_scene_destroy(scene);
        return 1;
    }
    ui_column_set_spacing(app.column, 14);
    apply_column_style(app.column, ui_color_from_hex(0x1E1A2F));
    ui_widget_set_bounds(ui_column_widget_mutable(app.column), 0, 0,
                         UI_FRAMEBUFFER_WIDTH, UI_FRAMEBUFFER_HEIGHT);

    ui_text_t *header = make_text("BareUI Widgets Showcase", ui_color_from_hex(0xFFD166),
                                  ui_color_from_hex(0x1E1A2F));
    ui_text_set_font(header, bareui_font_default());
    app.subtitle = make_text("Styles, rows, columns, buttons, scroll, and events.", ui_color_from_hex(0xF4F4F4),
                             ui_color_from_hex(0x1E1A2F));
    ui_column_add_control(app.column, ui_text_widget_mutable(header), false, "header");
    ui_column_add_control(app.column, ui_text_widget_mutable(app.subtitle), false, "subtitle");

    ui_column_set_spacing(app.column, 8);

    ui_row_t *actions = ui_row_create();
    ui_row_set_spacing(actions, 6);
    ui_widget_set_bounds(ui_row_widget_mutable(actions), 0, 0, UI_FRAMEBUFFER_WIDTH, 26);
    ui_button_t *btn1 = make_button("Launch");
    ui_button_t *btn2 = make_button("Compose");
    ui_button_t *btn3 = make_button("Inspect history");
    ui_button_set_on_click(btn1, on_action_click, &app);
    ui_button_set_on_click(btn2, on_action_click, &app);
    ui_button_set_on_click(btn3, on_action_click, &app);
    apply_row_style(actions, ui_color_from_hex(0x1E1A2F));
    ui_row_add_control(actions, ui_button_widget_mutable(btn1), true, "btn1");
    ui_row_add_control(actions, ui_button_widget_mutable(btn2), true, "btn2");
    ui_row_add_control(actions, ui_button_widget_mutable(btn3), true, "btn3");
    ui_column_add_control(app.column, ui_row_widget_mutable(actions), false, "row_actions");

    ui_button_t *theme_btn = make_button("Cycle Theme");
    ui_button_set_on_click(theme_btn, on_theme_click, &app);
    ui_row_t *theme_row = ui_row_create();
    ui_row_set_spacing(theme_row, 4);
    apply_row_style(theme_row, ui_color_from_hex(0x1E1A2F));
    ui_widget_set_bounds(ui_row_widget_mutable(theme_row), 0, 0, UI_FRAMEBUFFER_WIDTH, 24);
    ui_row_add_control(theme_row, ui_button_widget_mutable(theme_btn), true, "theme");
    ui_column_add_control(app.column, ui_row_widget_mutable(theme_row), false, "row_theme");

    ui_row_t *stats = ui_row_create();
    ui_row_set_spacing(stats, 10);
    apply_row_style(stats, ui_color_from_hex(0x1E1A2F));
    ui_widget_set_bounds(ui_row_widget_mutable(stats), 0, 0, UI_FRAMEBUFFER_WIDTH, 24);
    app.status = make_text("Interactions will appear here.", ui_color_from_hex(0xFFFFFF),
                           ui_color_from_hex(0x1E1A2F));
    app.clock = make_text("Uptime 0.0 s", ui_color_from_hex(0xAACFFF),
                         ui_color_from_hex(0x1E1A2F));
    ui_row_add_control(stats, ui_text_widget_mutable(app.status), true, "status");
    ui_row_add_control(stats, ui_text_widget_mutable(app.clock), true, "clock");
    ui_column_add_control(app.column, ui_row_widget_mutable(stats), false, "row_stats");

    ui_scene_set_root(scene, ui_column_widget_mutable(app.column));
    ui_scene_set_user_data(scene, &app);
    ui_scene_set_tick(scene, app_tick);

    ui_scene_run(scene);

    ui_scene_destroy(scene);
    ui_row_destroy(actions);
    ui_row_destroy(theme_row);
    ui_row_destroy(stats);
    ui_column_destroy(app.column);
    ui_text_destroy(header);
    ui_text_destroy(app.subtitle);
    ui_text_destroy(app.status);
    ui_text_destroy(app.clock);
    ui_button_destroy(btn1);
    ui_button_destroy(btn2);
    ui_button_destroy(btn3);
    ui_button_destroy(theme_btn);

    return 0;
}
