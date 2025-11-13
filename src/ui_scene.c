#define _POSIX_C_SOURCE 200809L

#include "ui_scene.h"

#include <stdlib.h>
#include <time.h>

struct ui_scene {
    ui_context_t *ctx;
    ui_widget_t *root;
    ui_scene_tick_fn tick;
    void *user_data;
    bool running;
};

static double ui_scene_time_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec * 1e-9;
}

ui_scene_t *ui_scene_create(const ui_hal_ops_t *hal)
{
    if (!hal) {
        return NULL;
    }
    ui_context_t *ctx = ui_context_create(hal);
    if (!ctx) {
        return NULL;
    }
    ui_scene_t *scene = calloc(1, sizeof(*scene));
    if (!scene) {
        ui_context_destroy(ctx);
        return NULL;
    }
    scene->ctx = ctx;
    scene->root = NULL;
    scene->tick = NULL;
    scene->user_data = hal->user_data;
    scene->running = false;
    return scene;
}

void ui_scene_destroy(ui_scene_t *scene)
{
    if (!scene) {
        return;
    }
    if (scene->root) {
        ui_widget_destroy_tree(scene->root);
    }
    if (scene->ctx) {
        ui_context_destroy(scene->ctx);
    }
    free(scene);
}

bool ui_scene_set_root(ui_scene_t *scene, ui_widget_t *root)
{
    if (!scene) {
        return false;
    }
    scene->root = root;
    return true;
}

ui_widget_t *ui_scene_root(const ui_scene_t *scene)
{
    return scene ? scene->root : NULL;
}

void ui_scene_set_tick(ui_scene_t *scene, ui_scene_tick_fn tick)
{
    if (scene) {
        scene->tick = tick;
    }
}

ui_scene_tick_fn ui_scene_tick(const ui_scene_t *scene)
{
    return scene ? scene->tick : NULL;
}

void ui_scene_set_user_data(ui_scene_t *scene, void *user_data)
{
    if (scene) {
        scene->user_data = user_data;
    }
}

void *ui_scene_user_data(const ui_scene_t *scene)
{
    return scene ? scene->user_data : NULL;
}

void ui_scene_request_exit(ui_scene_t *scene)
{
    if (scene) {
        scene->running = false;
    }
}

bool ui_scene_is_running(const ui_scene_t *scene)
{
    return scene ? scene->running : false;
}

void ui_scene_run(ui_scene_t *scene)
{
    if (!scene || !scene->ctx) {
        return;
    }
    scene->running = true;
    double previous = ui_scene_time_seconds();
    while (scene->running) {
        double now = ui_scene_time_seconds();
        double delta = now - previous;
        previous = now;

        bool saw_quit = false;
        ui_event_t event;
        while (ui_context_poll_event(scene->ctx, &event)) {
            if (event.type == UI_EVENT_QUIT) {
                saw_quit = true;
            }
            if (scene->root) {
                ui_widget_dispatch_event(scene->root, &event);
            }
        }
        if (saw_quit) {
            scene->running = false;
        }
        if (scene->tick && scene->running) {
            if (!scene->tick(scene, delta)) {
                scene->running = false;
            }
        }
        if (scene->root) {
            ui_widget_render_tree(scene->root, scene->ctx);
        }
        ui_context_render(scene->ctx);

        struct timespec wait = {0, 16666667};
        nanosleep(&wait, NULL);
    }
}
