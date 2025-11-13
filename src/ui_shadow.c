#include "ui_shadow.h"

void ui_shadow_render(ui_context_t *ctx, const ui_rect_t *bounds, const ui_shadow_t *shadow)
{
    if (!ctx || !bounds || !shadow || !shadow->enabled) {
        return;
    }
    ui_rect_t offset_bounds = {
        .x = bounds->x + shadow->offset_x,
        .y = bounds->y + shadow->offset_y,
        .width = bounds->width,
        .height = bounds->height
    };
    const ui_color_t color = shadow->color ? shadow->color : ui_color_from_hex(0x000000);
    ui_context_fill_rect(ctx, offset_bounds.x, offset_bounds.y, offset_bounds.width,
                         offset_bounds.height, color);
    if (shadow->blur > 0) {
        ui_context_fill_rect(ctx, offset_bounds.x - shadow->blur, offset_bounds.y - shadow->blur,
                             offset_bounds.width + shadow->blur * 2,
                             offset_bounds.height + shadow->blur * 2, color);
    }
}
