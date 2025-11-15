#include "ui_shadow.h"

void ui_shadow_render(ui_context_t *ctx, const ui_rect_t *bounds, const ui_box_shadow_t *shadow)
{
    if (!ctx || !bounds || !shadow || !shadow->enabled) {
        return;
    }
    int spread = shadow->spread_radius;
    ui_rect_t shadow_bounds = {
        .x = bounds->x + shadow->offset_x - spread,
        .y = bounds->y + shadow->offset_y - spread,
        .width = bounds->width + spread * 2,
        .height = bounds->height + spread * 2
    };
    const ui_color_t fallback_color = ui_color_from_hex(0xEED7C5); // Champagne pink light shadow
    const ui_color_t color = shadow->color ? shadow->color : fallback_color;
    ui_context_fill_rect(ctx, shadow_bounds.x, shadow_bounds.y, shadow_bounds.width,
                         shadow_bounds.height, color);
}
