#include "ui_button.h"
#include "ui_column.h"
#include "ui_progressring.h"
#include "ui_row.h"
#include "ui_scene.h"
#include "ui_text.h"
#include "ui_hal_test.h"
#include "ui_shadow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ui_scene_t *scene;
    ui_column_t *column;
    ui_text_t *status;
    ui_text_t *clock;
    ui_text_t *subtitle;
    ui_text_t *detail;
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
    int text_height = BAREUI_FONT_HEIGHT * 2 + 8;
    ui_widget_set_bounds(ui_text_widget_mutable(text), 0, 0, UI_FRAMEBUFFER_WIDTH, text_height);
    return text;
}

static ui_button_t *make_button(const char *label, int pad_h, int pad_v, int fixed_height)
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
    style.padding_left = style.padding_right = pad_h;
    style.padding_top = style.padding_bottom = pad_v;
    style.border_color = ui_color_from_hex(0x405070);
    style.border_width = 1;
    style.box_shadow.enabled = true;
    style.box_shadow.color = ui_color_from_hex(0x0F1018);
    style.box_shadow.offset_x = 1;
    style.box_shadow.offset_y = 1;
    style.box_shadow.spread_radius = 0;
    style.box_shadow.blur_radius = 1;
    style.border_sides = UI_BORDER_TOP | UI_BORDER_LEFT;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_FOREGROUND_COLOR |
                  UI_STYLE_FLAG_ACCENT_COLOR | UI_STYLE_FLAG_BORDER_COLOR |
                  UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BOX_SHADOW |
                  UI_STYLE_FLAG_BORDER_SIDES;
    ui_widget_set_style(ui_button_widget_mutable(button), &style);
    int button_height = fixed_height > 0 ? fixed_height :
                        BAREUI_FONT_HEIGHT + style.padding_top + style.padding_bottom + 4;
    ui_widget_set_bounds(ui_button_widget_mutable(button), 0, 0, 90, button_height);
    return button;
}

static ui_button_t *make_elevated_button(const char *label)
{
    ui_button_t *button = ui_button_create();
    if (!button) {
        return NULL;
    }
    ui_button_set_text(button, label);
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = ui_color_from_hex(0x3E3A6F);
    style.foreground_color = ui_color_from_hex(0xF7F7FF);
    style.padding_left = style.padding_right = 12;
    style.padding_top = style.padding_bottom = 6;
    style.box_shadow.enabled = true;
    style.box_shadow.color = ui_color_from_hex(0x05050E);
    style.box_shadow.offset_x = 0;
    style.box_shadow.offset_y = 3;
    style.box_shadow.blur_radius = 4;
    style.box_shadow.spread_radius = 1;
    style.box_shadow.blur_style = UI_SHADOW_BLUR_NORMAL;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_FOREGROUND_COLOR |
                  UI_STYLE_FLAG_BOX_SHADOW;
    ui_widget_set_style(ui_button_widget_mutable(button), &style);
    int button_height =
        BAREUI_FONT_HEIGHT + style.padding_top + style.padding_bottom + 4;
    ui_widget_set_bounds(ui_button_widget_mutable(button), 0, 0, 140, button_height);
    return button;
}

static ui_button_t *make_filled_button(const char *label, ui_color_t background,
                                       ui_color_t hover, ui_color_t pressed, int pad_h,
                                       int pad_v, int fixed_width)
{
    ui_button_t *button = ui_button_create();
    if (!button) {
        return NULL;
    }
    ui_button_set_text(button, label);
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.foreground_color = ui_color_from_hex(0xFFFFFF);
    style.accent_color = hover;
    style.padding_left = style.padding_right = pad_h;
    style.padding_top = style.padding_bottom = pad_v;
    style.border_width = 0;
    style.border_sides = 0;
    style.box_shadow.enabled = true;
    style.box_shadow.color = ui_color_from_hex(0x0A1625);
    style.box_shadow.offset_x = 0;
    style.box_shadow.offset_y = 2;
    style.box_shadow.blur_radius = 4;
    style.box_shadow.spread_radius = 1;
    style.box_shadow.blur_style = UI_SHADOW_BLUR_NORMAL;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_FOREGROUND_COLOR |
                  UI_STYLE_FLAG_ACCENT_COLOR | UI_STYLE_FLAG_BORDER_WIDTH |
                  UI_STYLE_FLAG_BORDER_SIDES | UI_STYLE_FLAG_BOX_SHADOW;
    ui_widget_set_style(ui_button_widget_mutable(button), &style);
    ui_button_set_hover_color(button, hover);
    ui_button_set_pressed_color(button, pressed);
    int button_height = BAREUI_FONT_HEIGHT + pad_v * 2 + 8;
    int button_width = fixed_width > 0 ? fixed_width : 120;
    ui_widget_set_bounds(ui_button_widget_mutable(button), 0, 0, button_width, button_height);
    return button;
}

static ui_button_t *make_outlined_button(const char *label, int pad_h, int pad_v, int fixed_height)
{
    ui_button_t *button = ui_button_create();
    if (!button) {
        return NULL;
    }
    ui_button_set_text(button, label);
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = ui_color_from_hex(0x1E1A2F);
    style.foreground_color = ui_color_from_hex(0xA1D8FF);
    style.accent_color = ui_color_from_hex(0x82C1FF);
    style.border_color = ui_color_from_hex(0x5DA4FF);
    style.border_width = 1;
    style.padding_left = style.padding_right = pad_h;
    style.padding_top = style.padding_bottom = pad_v;
    style.border_sides = UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_FOREGROUND_COLOR |
                  UI_STYLE_FLAG_ACCENT_COLOR | UI_STYLE_FLAG_BORDER_COLOR |
                  UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BORDER_SIDES;
    ui_widget_set_style(ui_button_widget_mutable(button), &style);
    int button_height = fixed_height > 0 ? fixed_height :
                        BAREUI_FONT_HEIGHT + style.padding_top + style.padding_bottom + 4;
    ui_widget_set_bounds(ui_button_widget_mutable(button), 0, 0, 120, button_height);
    return button;
}

static void apply_column_style(ui_column_t *column, ui_color_t background)
{
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.padding_top = style.padding_bottom = 6;
    style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_column_widget_mutable(column), &style);
}

static void apply_row_style(ui_row_t *row, ui_color_t background, int pad_v, int fixed_height)
{
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.padding_top = style.padding_bottom = pad_v;
    style.flags |= UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_row_widget_mutable(row), &style);
    int row_height = fixed_height > 0 ? fixed_height :
                     BAREUI_FONT_HEIGHT * 2 + style.padding_top + style.padding_bottom + 12;
    ui_widget_set_bounds(ui_row_widget_mutable(row), 0, 0, UI_FRAMEBUFFER_WIDTH, row_height);
}

static void update_status(app_state_t *app, const char *message)
{
    if (!app || !app->status) {
        return;
    }
    ui_text_set_value(app->status, message);
}

static void update_action_detail(app_state_t *app, const char *label)
{
    if (!app || !app->detail) {
        return;
    }
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Последнее действие: %s", label ? label : "—");
    ui_text_set_value(app->detail, buffer);
}

static void on_action_click(ui_button_t *button, void *user_data)
{
    app_state_t *app = user_data;
    if (!app) {
        return;
    }
    const char *label = ui_button_text(button);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Действие: %s", label ? label : "—");
    update_status(app, buffer);
    update_action_detail(app, label);
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
    update_status(app, "Theme changed / Тема обновлена");
    update_action_detail(app, "Смена темы");
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
        .detail = NULL,
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
    app.subtitle = make_text("Styles, rows, columns, buttons, scroll, and events. / Стили, строки, колонки, кнопки, прокрутка и события.",
                             ui_color_from_hex(0xF4F4F4),
                             ui_color_from_hex(0x1E1A2F));
    ui_column_add_control(app.column, ui_text_widget_mutable(header), false, "header");
    ui_column_add_control(app.column, ui_text_widget_mutable(app.subtitle), false, "subtitle");

    ui_column_set_spacing(app.column, 8);
    app.detail = make_text("Последнее действие: не выбрано.", ui_color_from_hex(0xC8EAFB),
                           ui_color_from_hex(0x1E1A2F));
    ui_text_set_font(app.detail, bareui_font_default());
    ui_column_add_control(app.column, ui_text_widget_mutable(app.detail), false, "detail");
    ui_text_t *language_sample = make_text(
        "Русский язык поддерживается.", ui_color_from_hex(0xB2FFE1),
        ui_color_from_hex(0x1E1A2F));
    ui_text_set_font(language_sample, bareui_font_default());
    ui_column_add_control(app.column, ui_text_widget_mutable(language_sample), false, "language");

    ui_row_t *actions = ui_row_create();
    ui_row_set_spacing(actions, 6);
    ui_button_t *btn1 = make_button("Запустить", 10, 6, 0);
    ui_button_t *btn2 = make_button("Создать", 10, 6, 0);
    ui_button_t *btn3 = make_button("История", 10, 6, 0);
    ui_button_set_on_click(btn1, on_action_click, &app);
    ui_button_set_on_click(btn2, on_action_click, &app);
    ui_button_set_on_click(btn3, on_action_click, &app);
    apply_row_style(actions, ui_color_from_hex(0x1E1A2F), 6, 40);
    ui_row_add_control(actions, ui_button_widget_mutable(btn1), false, "btn1");
    ui_row_add_control(actions, ui_button_widget_mutable(btn2), false, "btn2");
    ui_row_add_control(actions, ui_button_widget_mutable(btn3), false, "btn3");
    ui_column_add_control(app.column, ui_row_widget_mutable(actions), false, "row_actions");

    ui_text_t *outlined_caption = make_text(
        "Outlined buttons mark medium-emphasis actions that complement filled controls.",
        ui_color_from_hex(0xBCE0FF), ui_color_from_hex(0x1E1A2F));
    ui_text_set_font(outlined_caption, bareui_font_default());
    ui_column_add_control(app.column, ui_text_widget_mutable(outlined_caption), false, "outlined_caption");
    ui_row_t *outlined_row = ui_row_create();
    ui_row_set_spacing(outlined_row, 8);
    apply_row_style(outlined_row, ui_color_from_hex(0x1E1A2F), 5, 36);
    ui_button_t *outlined_btn = make_outlined_button("Подробнее", 12, 6, 0);
    ui_button_set_on_click(outlined_btn, on_action_click, &app);
    ui_row_add_control(outlined_row, ui_button_widget_mutable(outlined_btn), true, "btn_outlined");
    ui_column_add_control(app.column, ui_row_widget_mutable(outlined_row), false, "row_outlined");

    ui_button_t *theme_btn = make_button("Сменить тему", 10, 5, 32);
    ui_button_set_on_click(theme_btn, on_theme_click, &app);
    ui_row_t *theme_row = ui_row_create();
    ui_row_set_spacing(theme_row, 4);
    apply_row_style(theme_row, ui_color_from_hex(0x1E1A2F), 5, 36);
    ui_row_add_control(theme_row, ui_button_widget_mutable(theme_btn), true, "theme");
    ui_column_add_control(app.column, ui_row_widget_mutable(theme_row), false, "row_theme");

    ui_text_t *filled_hint = make_text(
        "Filled buttons highlight critical actions (Save, Join now, Confirm).",
        ui_color_from_hex(0xFFD166), ui_color_from_hex(0x1E1A2F));
    ui_text_set_font(filled_hint, bareui_font_default());
    ui_column_add_control(app.column, ui_text_widget_mutable(filled_hint), false, "filled_hint");

    ui_row_t *final_actions = ui_row_create();
    ui_row_set_spacing(final_actions, 8);
    apply_row_style(final_actions, ui_color_from_hex(0x1E1A2F), 6, 48);
    ui_button_t *save_btn = make_filled_button("Сохранить",
                                               ui_color_from_hex(0x1CC964),
                                               ui_color_from_hex(0x4AD687),
                                               ui_color_from_hex(0x14834D), 10, 6, 100);
    ui_button_t *join_btn = make_filled_button("Присоединиться",
                                               ui_color_from_hex(0x7C4DFF),
                                               ui_color_from_hex(0xA27EFF),
                                               ui_color_from_hex(0x5B32B7), 10, 6, 100);
    ui_button_t *confirm_btn = make_filled_button("Подтвердить",
                                                  ui_color_from_hex(0xFF6F43),
                                                  ui_color_from_hex(0xFFA07C),
                                                  ui_color_from_hex(0xC04832), 10, 6, 100);
    ui_button_set_on_click(save_btn, on_action_click, &app);
    ui_button_set_on_click(join_btn, on_action_click, &app);
    ui_button_set_on_click(confirm_btn, on_action_click, &app);
    ui_row_add_control(final_actions, ui_button_widget_mutable(save_btn), false, "filled_save");
    ui_row_add_control(final_actions, ui_button_widget_mutable(join_btn), false, "filled_join");
    ui_row_add_control(final_actions, ui_button_widget_mutable(confirm_btn), false,
                       "filled_confirm");
    ui_column_add_control(app.column, ui_row_widget_mutable(final_actions), false,
                          "row_filled_actions");

    ui_text_t *elevated_note = make_text(
        "Elevated buttons are filled tonal buttons with a shadow. Use them only when "
        "they must separate from patterned backgrounds.",
        ui_color_from_hex(0xB6CCFF), ui_color_from_hex(0x141521));
    ui_text_set_font(elevated_note, bareui_font_default());
    ui_column_add_control(app.column, ui_text_widget_mutable(elevated_note), false,
                         "elevated_note");
    ui_button_t *elevated_btn = make_elevated_button("Elevated action");
    ui_button_set_on_click(elevated_btn, on_action_click, &app);
    ui_row_t *elevated_row = ui_row_create();
    ui_row_set_spacing(elevated_row, 4);
    apply_row_style(elevated_row, ui_color_from_hex(0x141523), 6, 44);
    ui_row_add_control(elevated_row, ui_button_widget_mutable(elevated_btn), true,
                       "elevated");
    ui_column_add_control(app.column, ui_row_widget_mutable(elevated_row), false,
                         "row_elevated");

    ui_row_t *stats = ui_row_create();
    ui_row_set_spacing(stats, 10);
    apply_row_style(stats, ui_color_from_hex(0x1E1A2F), 4, 32);
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
    ui_row_destroy(outlined_row);
    ui_row_destroy(theme_row);
    ui_row_destroy(final_actions);
    ui_row_destroy(elevated_row);
    ui_row_destroy(stats);
    ui_column_destroy(app.column);
    ui_text_destroy(header);
    ui_text_destroy(app.subtitle);
    ui_text_destroy(app.detail);
    ui_text_destroy(language_sample);
    ui_text_destroy(filled_hint);
    ui_text_destroy(outlined_caption);
    ui_text_destroy(elevated_note);
    ui_text_destroy(app.status);
    ui_text_destroy(app.clock);
    ui_button_destroy(btn1);
    ui_button_destroy(btn2);
    ui_button_destroy(btn3);
    ui_button_destroy(outlined_btn);
    ui_button_destroy(theme_btn);
    ui_button_destroy(save_btn);
    ui_button_destroy(join_btn);
    ui_button_destroy(confirm_btn);
    ui_button_destroy(elevated_btn);

    return 0;
}
