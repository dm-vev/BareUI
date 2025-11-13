#ifndef UI_SCENE_H
#define UI_SCENE_H

#include "ui_primitives.h"
#include "ui_widget.h"

#include <stdbool.h>

typedef struct ui_scene ui_scene_t;

typedef bool (*ui_scene_tick_fn)(ui_scene_t *scene, double delta_seconds);

ui_scene_t *ui_scene_create(const ui_hal_ops_t *hal);
void ui_scene_destroy(ui_scene_t *scene);

bool ui_scene_set_root(ui_scene_t *scene, ui_widget_t *root);
ui_widget_t *ui_scene_root(const ui_scene_t *scene);

void ui_scene_set_tick(ui_scene_t *scene, ui_scene_tick_fn tick);
ui_scene_tick_fn ui_scene_tick(const ui_scene_t *scene);

void ui_scene_set_user_data(ui_scene_t *scene, void *user_data);
void *ui_scene_user_data(const ui_scene_t *scene);

void ui_scene_request_exit(ui_scene_t *scene);
bool ui_scene_is_running(const ui_scene_t *scene);

void ui_scene_run(ui_scene_t *scene);

#endif
