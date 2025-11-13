#ifndef UI_SHADOW_H
#define UI_SHADOW_H

#include "ui_primitives.h"

#include <stdbool.h>

typedef enum {
    UI_SHADOW_BLUR_NORMAL,
    UI_SHADOW_BLUR_SOLID
} ui_shadow_blur_style_t;

typedef struct {
    bool enabled;
    int offset_x;
    int offset_y;
    int blur_radius;
    int spread_radius;
    ui_shadow_blur_style_t blur_style;
    ui_color_t color;
} ui_box_shadow_t;

void ui_shadow_render(ui_context_t *ctx, const ui_rect_t *bounds, const ui_box_shadow_t *shadow);

#endif
