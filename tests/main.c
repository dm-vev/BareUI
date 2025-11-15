#include "ui_appbar.h"
#include "ui_button.h"
#include "ui_column.h"
#include "ui_radio.h"
#include "ui_row.h"
#include "ui_scene.h"
#include "ui_slider.h"
#include "ui_progressbar.h"
#include "ui_switch.h"
#include "ui_text.h"
#include "ui_tab.h"
#include "ui_hal_test.h"
#include "ui_shadow.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    ui_scene_t *scene;
    ui_slider_t *slider;
    ui_progressbar_t *progress;
} app_state_t;

static void on_radio_group_change(ui_radio_group_t *group, int32_t value, void *user_data)
{
    (void)group;
    (void)user_data;
    if (value == UI_RADIO_VALUE_NONE) {
        printf("radio cleared\n");
    } else {
        printf("radio selected: %d\n", value);
    }
}

static void on_slider_change(ui_slider_t *slider, double value, void *user_data)
{
    app_state_t *app = user_data;
    double max_value = ui_slider_max(slider);
    double normalized = max_value > 0.0 ? value / max_value : value;
    ui_progressbar_set_value(app->progress, normalized);
}

int main(void) {
    const ui_hal_ops_t *hal = ui_hal_test_sdl_ops();
    if (!hal)
        return 1;

    ui_scene_t *scene = ui_scene_create(hal);
    if (!scene)
        return 1;

    app_state_t app = { .scene = scene };
    ui_tabs_t *tabs = NULL;
    ui_text_t *overview_content = NULL;
    ui_text_t *details_content = NULL;

    ui_column_t *root = ui_column_create();
    ui_column_set_spacing(root, 20);
    ui_widget_set_bounds(ui_column_widget_mutable(root), 0, 0, UI_FRAMEBUFFER_WIDTH, UI_FRAMEBUFFER_HEIGHT);

    // ---------- APPBAR ----------
    ui_appbar_t *appbar = ui_appbar_create();
    ui_text_t *title = ui_text_create();
    ui_text_set_value(title, "Material AppBar");
    ui_text_set_align(title, UI_TEXT_ALIGN_CENTER);
    ui_appbar_set_title(appbar, ui_text_widget_mutable(title));
    ui_appbar_set_center_title(appbar, true);
    ui_appbar_set_background_color(appbar, ui_color_from_hex(0x1F1B2D));
    ui_appbar_set_title_spacing(appbar, 18);
    ui_appbar_set_toolbar_height(appbar, 64);

    ui_button_t *menu = ui_button_create();
    ui_button_set_text(menu, "Menu");
    ui_widget_set_bounds(ui_button_widget_mutable(menu), 0, 0, 44, 44);
    ui_appbar_set_leading(appbar, ui_button_widget_mutable(menu));

    ui_button_t *search = ui_button_create();
    ui_button_set_text(search, "Search");
    ui_widget_set_bounds(ui_button_widget_mutable(search), 0, 0, 36, 36);
    ui_appbar_add_action(appbar, ui_button_widget_mutable(search), "search");

    ui_button_t *more = ui_button_create();
    ui_button_set_text(more, "More");
    ui_widget_set_bounds(ui_button_widget_mutable(more), 0, 0, 36, 36);
    ui_appbar_add_action(appbar, ui_button_widget_mutable(more), "overflow");

    ui_widget_set_bounds(ui_appbar_widget_mutable(appbar), 0, 0, UI_FRAMEBUFFER_WIDTH, 64);
    ui_column_add_control(root, ui_appbar_widget_mutable(appbar), false, "appbar");

    // ---------- СЛАЙДЕР ----------
    ui_slider_t *slider = ui_slider_create();
    app.slider = slider;

    ui_slider_set_min(slider, 0);
    ui_slider_set_max(slider, 100);
    ui_slider_set_value(slider, 50);
    ui_slider_set_label(slider, "{value}%");

    ui_slider_set_on_change(slider, on_slider_change, &app);
    ui_widget_set_bounds(ui_slider_widget_mutable(slider), 0, 0, UI_FRAMEBUFFER_WIDTH, 50);

    // ---------- ПРОГРЕССБАР ----------
    ui_progressbar_t *progress = ui_progressbar_create();
    app.progress = progress;

    double slider_max_value = ui_slider_max(slider);
    double slider_value = ui_slider_value(slider);
    double initial_progress = slider_max_value > 0.0 ? slider_value / slider_max_value : slider_value;
    ui_progressbar_set_value(progress, initial_progress);

    ui_widget_set_bounds(ui_progressbar_widget_mutable(progress), 0, 0, UI_FRAMEBUFFER_WIDTH, 40);

    ui_radio_group_t *radio_group = ui_radio_group_create();
    ui_radio_t *radio_material = ui_radio_create();
    ui_radio_t *radio_cupertino = ui_radio_create();

    ui_radio_set_label(radio_material, "Material style");
    ui_radio_set_value(radio_material, 1);
    ui_radio_set_group(radio_material, radio_group);

    ui_radio_set_label(radio_cupertino, "Cupertino style");
    ui_radio_set_value(radio_cupertino, 2);
    ui_radio_set_group(radio_cupertino, radio_group);
    ui_radio_set_adaptive(radio_cupertino, true);

    ui_radio_group_set_on_change(radio_group, on_radio_group_change, NULL);
    ui_radio_group_set_value(radio_group, 1);

    ui_widget_set_bounds(ui_radio_widget_mutable(radio_material), 0, 0, UI_FRAMEBUFFER_WIDTH, 30);
    ui_widget_set_bounds(ui_radio_widget_mutable(radio_cupertino), 0, 0, UI_FRAMEBUFFER_WIDTH, 30);

    // добавляем в колонку
    ui_column_add_control(root, ui_slider_widget_mutable(slider), false, "slider");
    ui_column_add_control(root, ui_progressbar_widget_mutable(progress), false, "progress");
    ui_column_add_control(root, ui_radio_widget_mutable(radio_material), false, "radio-material");
    ui_column_add_control(root, ui_radio_widget_mutable(radio_cupertino), false, "radio-cupertino");

    tabs = ui_tabs_create();
    overview_content = ui_text_create();
    details_content = ui_text_create();
    if (tabs) {
        ui_tab_t *overview_tab = ui_tab_create();
        ui_tab_t *details_tab = ui_tab_create();

        ui_tab_set_text(overview_tab, "Overview");
        ui_tab_set_text(details_tab, "Details");

        ui_text_set_value(overview_content, "Overview content highlights the most frequently used controls.");
        ui_text_set_align(overview_content, UI_TEXT_ALIGN_LEFT);
        ui_tab_set_content(overview_tab, ui_text_widget_mutable(overview_content));

        ui_text_set_value(details_content, "Details content surfaces secondary options and context info.");
        ui_text_set_align(details_content, UI_TEXT_ALIGN_CENTER);
        ui_tab_set_content(details_tab, ui_text_widget_mutable(details_content));

        ui_tabs_add_tab(tabs, overview_tab);
        ui_tabs_add_tab(tabs, details_tab);

        ui_tabs_set_indicator_color(tabs, ui_color_from_hex(0x3D5AFE));
        ui_tabs_set_unselected_label_color(tabs, ui_color_from_hex(0xAAAAAA));
        ui_tabs_set_padding(tabs, (ui_padding_t){ .top = 6, .bottom = 4, .left = 12, .right = 12 });
        ui_tabs_set_label_padding(tabs, (ui_padding_t){ .top = 3, .bottom = 3, .left = 10, .right = 10 });
        ui_widget_set_bounds(ui_tabs_widget_mutable(tabs), 0, 0, UI_FRAMEBUFFER_WIDTH, 120);
        ui_column_add_control(root, ui_tabs_widget_mutable(tabs), false, "tabs");
    }

    ui_scene_set_root(scene, ui_column_widget_mutable(root));
    ui_scene_run(scene);

    // очистка
    ui_progressbar_destroy(progress);
    ui_slider_destroy(slider);
    ui_radio_destroy(radio_material);
    ui_radio_destroy(radio_cupertino);
    ui_radio_group_destroy(radio_group);
    ui_button_destroy(more);
    ui_button_destroy(search);
    ui_button_destroy(menu);
    ui_text_destroy(title);
    ui_appbar_destroy(appbar);
    ui_tabs_destroy(tabs);
    ui_text_destroy(overview_content);
    ui_text_destroy(details_content);
    ui_column_destroy(root);
    ui_scene_destroy(scene);

    return 0;
}
