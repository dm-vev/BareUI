#include "ui_core.h"
#include "ui_hal_test.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
    const ui_hal_ops_t *hal = ui_hal_test_sdl_ops();
    ui_context_t *ctx = ui_context_create(hal);
    if (!ctx) {
        return 1;
    }

    bool running = true;
    bool pressed = false;
    int last_x = -1;
    int last_y = -1;

    while (running) {
        ui_event_t event;
        while (ui_context_poll_event(ctx, &event)) {
            switch (event.type) {
            case UI_EVENT_TOUCH_DOWN:
                pressed = true;
                last_x = event.data.touch.x;
                last_y = event.data.touch.y;
                break;
            case UI_EVENT_TOUCH_UP:
                pressed = false;
                break;
            case UI_EVENT_TOUCH_MOVE:
                if (pressed) {
                    last_x = event.data.touch.x;
                    last_y = event.data.touch.y;
                }
                break;
            case UI_EVENT_KEY_DOWN:
                if (event.data.key.keycode == 'q' || event.data.key.keycode == 'Q') {
                    running = false;
                }
                break;
            case UI_EVENT_QUIT:
                running = false;
                break;
            default:
                break;
            }
        }

        ui_context_clear(ctx, ui_color_from_hex(0x0B1622));
        ui_context_fill_rect(ctx, 16, 16, 288, 104, ui_color_from_hex(0x162B47));
        ui_context_draw_text(ctx, 28, 26, "Platform-neutral UI test", ui_color_from_hex(0xE0F4FF));
        ui_context_draw_text(ctx, 28, 40, "Click or tap around the window.", ui_color_from_hex(0xB8D9FF));
        ui_context_draw_text(ctx, 28, 54, "Press q to exit (Esc works too).", ui_color_from_hex(0x90B6FF));

        char coords[64];
        if (last_x >= 0 && last_y >= 0) {
            snprintf(coords, sizeof(coords), "Last touch: %3d, %3d", last_x, last_y);
        } else {
            snprintf(coords, sizeof(coords), "Last touch: - , -");
        }
        ui_context_draw_text(ctx, 28, 80, coords, ui_color_from_hex(0xF0F0F0));

        if (last_x >= 0 && last_y >= 0) {
            ui_color_t indicator = pressed ? ui_color_from_hex(0xFF5F3E) : ui_color_from_hex(0x2CE4A0);
            ui_context_fill_rect(ctx, last_x - 8, last_y - 8, 16, 16, indicator);
        }

        ui_context_render(ctx);

        struct timespec wait = {0, 16 * 1000000};
        nanosleep(&wait, NULL);
    }

    ui_context_destroy(ctx);
    return 0;
}
