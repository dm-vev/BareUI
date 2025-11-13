#ifndef UI_SHADOW_H
#define UI_SHADOW_H

#include "ui_primitives.h"
#include "ui_widget.h"

#include <stdbool.h>

typedef struct {
    bool enabled;
    int offset_x;
    int offset_y;
    ui_color_t color;
    int blur;
} ui_shadow_t;

void ui_shadow_render(ui_context_t *ctx, const ui_rect_t *bounds, const ui_shadow_t *shadow);

#endif
