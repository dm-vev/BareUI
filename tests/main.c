#include "ui_column.h"
#include "ui_row.h"
#include "ui_scene.h"
#include "ui_slider.h"
#include "ui_progressbar.h"
#include "ui_switch.h"
#include "ui_text.h"
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

    ui_column_t *root = ui_column_create();
    ui_column_set_spacing(root, 20);
    ui_widget_set_bounds(ui_column_widget_mutable(root), 0, 0, UI_FRAMEBUFFER_WIDTH, UI_FRAMEBUFFER_HEIGHT);

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

    // добавляем в колонку
    ui_column_add_control(root, ui_slider_widget_mutable(slider), false, "slider");
    ui_column_add_control(root, ui_progressbar_widget_mutable(progress), false, "progress");

    ui_scene_set_root(scene, ui_column_widget_mutable(root));
    ui_scene_run(scene);

    // очистка
    ui_progressbar_destroy(progress);
    ui_slider_destroy(slider);
    ui_column_destroy(root);
    ui_scene_destroy(scene);

    return 0;
}
