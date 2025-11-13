#include "ui_shadow.h"

void ui_shadow_render(ui_context_t *ctx, const ui_rect_t *bounds, const ui_box_shadow_t *shadow)
{
    if (!ctx || !bounds || !shadow || !shadow->enabled) {
        return;
    }
    int spread = shadow->spread_radius;
    ui_rect_t offset_bounds = {
        .x = bounds->x + shadow->offset_x - spread,
        .y = bounds->y + shadow->offset_y - spread,
        .width = bounds->width + spread * 2,
        .height = bounds->height + spread * 2
    };
    const ui_color_t color = shadow->color ? shadow->color : ui_color_from_hex(0x000000);
    ui_context_fill_rect(ctx, offset_bounds.x, offset_bounds.y, offset_bounds.width,
                         offset_bounds.height, color);
    if (shadow->blur_radius > 0) {
        int blur_expr = shadow->blur_radius;
        ui_context_fill_rect(ctx, offset_bounds.x - blur_expr, offset_bounds.y - blur_expr,
                             offset_bounds.width + blur_expr * 2,
                             offset_bounds.height + blur_expr * 2, color);
    }
}
