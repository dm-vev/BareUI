#include "ui_hal_test.h"

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define UI_TEST_SCALE 1
#define UI_TEST_WIDTH (UI_FRAMEBUFFER_WIDTH * UI_TEST_SCALE)
#define UI_TEST_HEIGHT (UI_FRAMEBUFFER_HEIGHT * UI_TEST_SCALE)

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint32_t *scaled_buffer;
    bool running;
} hal_sdl_state_t;

static inline uint32_t hal_color_to_argb(ui_color_t pixel)
{
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

static void hal_process_events(ui_context_t *ctx, hal_sdl_state_t *state)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ui_event_t ui_evt = {0};
        bool valid = true;
        switch (event.type) {
        case SDL_QUIT:
            ui_evt.type = UI_EVENT_QUIT;
            state->running = false;
            break;
        case SDL_MOUSEBUTTONDOWN:
            ui_evt.type = UI_EVENT_TOUCH_DOWN;
            ui_evt.data.touch.x = event.button.x / UI_TEST_SCALE;
            ui_evt.data.touch.y = event.button.y / UI_TEST_SCALE;
            break;
        case SDL_MOUSEBUTTONUP:
            ui_evt.type = UI_EVENT_TOUCH_UP;
            ui_evt.data.touch.x = event.button.x / UI_TEST_SCALE;
            ui_evt.data.touch.y = event.button.y / UI_TEST_SCALE;
            break;
        case SDL_MOUSEMOTION:
            ui_evt.type = UI_EVENT_TOUCH_MOVE;
            ui_evt.data.touch.x = event.motion.x / UI_TEST_SCALE;
            ui_evt.data.touch.y = event.motion.y / UI_TEST_SCALE;
            break;
        case SDL_KEYDOWN:
            ui_evt.type = UI_EVENT_KEY_DOWN;
            ui_evt.data.key.keycode = (uint32_t)event.key.keysym.sym;
            break;
        case SDL_KEYUP:
            ui_evt.type = UI_EVENT_KEY_UP;
            ui_evt.data.key.keycode = (uint32_t)event.key.keysym.sym;
            break;
        default:
            valid = false;
            break;
        }
        if (valid) {
            ui_context_post_event(ctx, &ui_evt);
        }
    }
}

static bool hal_sdl_init(ui_context_t *ctx)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return false;
    }

    hal_sdl_state_t *state = calloc(1, sizeof(*state));
    if (!state) {
        SDL_Quit();
        return false;
    }

    state->window = SDL_CreateWindow("BareUI test",
                                     SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED,
                                     UI_TEST_WIDTH,
                                     UI_TEST_HEIGHT,
                                     SDL_WINDOW_SHOWN);
    if (!state->window) {
        free(state);
        SDL_Quit();
        return false;
    }

    state->renderer = SDL_CreateRenderer(state->window, -1, SDL_RENDERER_ACCELERATED);
    if (!state->renderer) {
        SDL_DestroyWindow(state->window);
        free(state);
        SDL_Quit();
        return false;
    }

    state->texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       UI_TEST_WIDTH, UI_TEST_HEIGHT);
    if (!state->texture) {
        SDL_DestroyRenderer(state->renderer);
        SDL_DestroyWindow(state->window);
        free(state);
        SDL_Quit();
        return false;
    }

    state->scaled_buffer = calloc(UI_TEST_WIDTH * UI_TEST_HEIGHT, sizeof(uint32_t));
    if (!state->scaled_buffer) {
        SDL_DestroyTexture(state->texture);
        SDL_DestroyRenderer(state->renderer);
        SDL_DestroyWindow(state->window);
        free(state);
        SDL_Quit();
        return false;
    }

    state->running = true;
    ui_context_set_user_data(ctx, state);
    return true;
}

static void hal_sdl_deinit(ui_context_t *ctx)
{
    hal_sdl_state_t *state = ui_context_user_data(ctx);
    if (!state) {
        SDL_Quit();
        return;
    }

    state->running = false;
    if (state->scaled_buffer) {
        free(state->scaled_buffer);
    }
    if (state->texture) {
        SDL_DestroyTexture(state->texture);
    }
    if (state->renderer) {
        SDL_DestroyRenderer(state->renderer);
    }
    if (state->window) {
        SDL_DestroyWindow(state->window);
    }
    free(state);
    ui_context_set_user_data(ctx, NULL);
    SDL_Quit();
}

static void hal_sdl_commit(ui_context_t *ctx, const ui_color_t *framebuffer)
{
    hal_sdl_state_t *state = ui_context_user_data(ctx);
    if (!state || !state->running) {
        return;
    }

    hal_process_events(ctx, state);

    uint32_t *dest = state->scaled_buffer;
    for (int y = 0; y < UI_FRAMEBUFFER_HEIGHT; ++y) {
        for (int x = 0; x < UI_FRAMEBUFFER_WIDTH; ++x) {
            uint32_t packed = hal_color_to_argb(framebuffer[y * UI_FRAMEBUFFER_WIDTH + x]);
            for (int dy = 0; dy < UI_TEST_SCALE; ++dy) {
                uint32_t *row = dest + (y * UI_TEST_SCALE + dy) * UI_TEST_WIDTH + x * UI_TEST_SCALE;
                for (int dx = 0; dx < UI_TEST_SCALE; ++dx) {
                    row[dx] = packed;
                }
            }
        }
    }

    SDL_UpdateTexture(state->texture, NULL, state->scaled_buffer, UI_TEST_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(state->renderer);
    SDL_RenderCopy(state->renderer, state->texture, NULL, NULL);
    SDL_RenderPresent(state->renderer);
}

static const ui_hal_ops_t test_ops = {
    .user_data = NULL,
    .init = hal_sdl_init,
    .deinit = hal_sdl_deinit,
    .commit_frame = hal_sdl_commit
};

const ui_hal_ops_t *ui_hal_test_sdl_ops(void)
{
    return &test_ops;
}
