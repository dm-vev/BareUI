#include "ui_button.h"
#include "ui_column.h"
#include "ui_row.h"
#include "ui_scene.h"
#include "ui_text.h"
#include "ui_hal_test.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    ui_text_t *display;
    char buffer[64];
    double accumulator;
    char pending_operator;
    bool waiting_for_new_entry;
    bool entry_has_decimal;
    bool error;
} calculator_state_t;

typedef struct {
    calculator_state_t *state;
    const char *label;
} calculator_button_ctx_t;

typedef struct {
    uint32_t seashell;
    uint32_t champagne;
    uint32_t rosy_brown;
    uint32_t burnt_sienna;
    uint32_t redwood;
    uint32_t dark_text;
} classic_noir_palette_hex_t;

typedef struct {
    ui_color_t seashell;
    ui_color_t champagne;
    ui_color_t rosy_brown;
    ui_color_t burnt_sienna;
    ui_color_t redwood;
    ui_color_t dark_text;
} classic_noir_palette_t;

static const classic_noir_palette_hex_t CLASSIC_NOIR_PALETTE_HEX = {
    .seashell = 0xEEE2DF,
    .champagne = 0xEED7C5,
    .rosy_brown = 0xC89F9C,
    .burnt_sienna = 0xC97C5D,
    .redwood = 0xB36A5E,
    .dark_text = 0x1A1A1A
};

static void calculator_refresh_display(calculator_state_t *state)
{
    if (!state || !state->display) {
        return;
    }
    ui_text_set_value(state->display, state->buffer);
}

static void calculator_set_buffer(calculator_state_t *state, const char *value)
{
    if (!state || !value) {
        return;
    }
    size_t len = strlen(value);
    if (len >= sizeof(state->buffer)) {
        len = sizeof(state->buffer) - 1;
    }
    memcpy(state->buffer, value, len);
    state->buffer[len] = '\0';
    state->entry_has_decimal = (strchr(state->buffer, '.') != NULL);
    calculator_refresh_display(state);
}

static void calculator_set_buffer_from_double(calculator_state_t *state, double value)
{
    if (!state) {
        return;
    }
    char formatted[64];
    snprintf(formatted, sizeof(formatted), "%.12g", value);
    calculator_set_buffer(state, formatted);
}

static void calculator_set_error(calculator_state_t *state)
{
    if (!state) {
        return;
    }
    state->error = true;
    calculator_set_buffer(state, "ERR");
}

static void calculator_reset(calculator_state_t *state)
{
    if (!state) {
        return;
    }
    state->accumulator = 0.0;
    state->pending_operator = 0;
    state->waiting_for_new_entry = true;
    state->entry_has_decimal = false;
    state->error = false;
    calculator_set_buffer(state, "0");
}

static void calculator_input_digit(calculator_state_t *state, char digit)
{
    if (!state) {
        return;
    }
    if (state->error) {
        calculator_reset(state);
    }
    if (state->waiting_for_new_entry) {
        state->waiting_for_new_entry = false;
        state->entry_has_decimal = (digit == '.');
        if (digit == '.') {
            strcpy(state->buffer, "0.");
        } else {
            state->buffer[0] = digit;
            state->buffer[1] = '\0';
        }
        calculator_refresh_display(state);
        return;
    }

    size_t len = strlen(state->buffer);
    if (len >= sizeof(state->buffer) - 1) {
        return;
    }
    if (digit == '.') {
        if (state->entry_has_decimal) {
            return;
        }
        state->buffer[len] = '.';
        state->buffer[len + 1] = '\0';
        state->entry_has_decimal = true;
        calculator_refresh_display(state);
        return;
    }

    if (len == 1 && state->buffer[0] == '0' && !state->entry_has_decimal) {
        state->buffer[0] = digit;
        state->buffer[1] = '\0';
        calculator_refresh_display(state);
        return;
    }

    state->buffer[len] = digit;
    state->buffer[len + 1] = '\0';
    calculator_refresh_display(state);
}

static bool calculator_apply_pending_operation(calculator_state_t *state)
{
    if (!state || state->error) {
        return false;
    }
    double current = strtod(state->buffer, NULL);
    switch (state->pending_operator) {
        case 0:
            state->accumulator = current;
            break;
        case '+':
            state->accumulator += current;
            break;
        case '-':
            state->accumulator -= current;
            break;
        case '*':
            state->accumulator *= current;
            break;
        case '/':
            if (current == 0.0) {
                calculator_set_error(state);
                return false;
            }
            state->accumulator /= current;
            break;
        default:
            break;
    }

    if (isnan(state->accumulator) || isinf(state->accumulator)) {
        calculator_set_error(state);
        return false;
    }

    calculator_set_buffer_from_double(state, state->accumulator);
    state->waiting_for_new_entry = true;
    return true;
}

static void calculator_set_operator(calculator_state_t *state, char op)
{
    if (!state || state->error) {
        return;
    }
    if (!state->waiting_for_new_entry || state->pending_operator == 0) {
        calculator_apply_pending_operation(state);
    }
    state->pending_operator = op;
    state->waiting_for_new_entry = true;
    state->entry_has_decimal = false;
}

static void calculator_commit_equal(calculator_state_t *state)
{
    if (!state || state->error) {
        return;
    }
    if (state->pending_operator != 0) {
        calculator_apply_pending_operation(state);
        state->pending_operator = 0;
    }
    state->waiting_for_new_entry = true;
    state->entry_has_decimal = false;
}

static void calculator_toggle_sign(calculator_state_t *state)
{
    if (!state || state->error) {
        return;
    }
    if (strcmp(state->buffer, "0") == 0) {
        return;
    }
    if (state->buffer[0] == '-') {
        memmove(state->buffer, state->buffer + 1, strlen(state->buffer));
    } else {
        size_t len = strlen(state->buffer);
        if (len + 1 >= sizeof(state->buffer)) {
            return;
        }
        memmove(state->buffer + 1, state->buffer, len + 1);
        state->buffer[0] = '-';
    }
    calculator_refresh_display(state);
}

static void calculator_apply_percent(calculator_state_t *state)
{
    if (!state || state->error) {
        return;
    }
    double value = strtod(state->buffer, NULL) / 100.0;
    if (isnan(value) || isinf(value)) {
        calculator_set_error(state);
        return;
    }
    calculator_set_buffer_from_double(state, value);
    state->waiting_for_new_entry = true;
    state->entry_has_decimal = strchr(state->buffer, '.') != NULL;
}

static void calculator_handle_input(calculator_state_t *state, const char *label)
{
    if (!state || !label) {
        return;
    }
    if (strcmp(label, "C") == 0) {
        calculator_reset(state);
        return;
    }
    if (strcmp(label, "+/-") == 0) {
        calculator_toggle_sign(state);
        return;
    }
    if (strcmp(label, "%") == 0) {
        calculator_apply_percent(state);
        return;
    }
    if (strcmp(label, "=") == 0) {
        calculator_commit_equal(state);
        return;
    }
    if (strcmp(label, "+") == 0) {
        calculator_set_operator(state, '+');
        return;
    }
    if (strcmp(label, "-") == 0) {
        calculator_set_operator(state, '-');
        return;
    }
    if (strcmp(label, "*") == 0) {
        calculator_set_operator(state, '*');
        return;
    }
    if (strcmp(label, "/") == 0) {
        calculator_set_operator(state, '/');
        return;
    }
    if (strcmp(label, ".") == 0) {
        calculator_input_digit(state, '.');
        return;
    }
    if (label[0] && label[1] == '\0' && isdigit((unsigned char)label[0])) {
        calculator_input_digit(state, label[0]);
    }
}

static void calculator_on_button(ui_button_t *button, void *user_data)
{
    (void)button;
    calculator_button_ctx_t *ctx = user_data;
    if (!ctx) {
        return;
    }
    calculator_handle_input(ctx->state, ctx->label);
}

static void calculator_configure_button(ui_button_t *button, ui_color_t base, ui_color_t hover,
                                        ui_color_t pressed, ui_color_t text_color, ui_color_t shadow_color)
{
    if (!button) {
        return;
    }
    ui_button_set_text_color(button, text_color);
    ui_button_set_filled_style(button, base, hover, pressed);
    const ui_style_t *applied_style = ui_widget_style(ui_button_widget_mutable(button));
    if (!applied_style) {
        return;
    }
    ui_style_t stripped = *applied_style;
    stripped.box_shadow.enabled = true;
    stripped.box_shadow.color = shadow_color;
    stripped.box_shadow.offset_x = 0;
    stripped.box_shadow.offset_y = 1;
    stripped.box_shadow.blur_radius = 1;
    stripped.box_shadow.spread_radius = 0;
    stripped.flags |= UI_STYLE_FLAG_BOX_SHADOW;
    ui_widget_set_style(ui_button_widget_mutable(button), &stripped);
}

static void calculator_pick_button_colors(const classic_noir_palette_t *palette, const char *label,
                                          ui_color_t *out_base, ui_color_t *out_hover, ui_color_t *out_pressed)
{
    if (!palette || !out_base || !out_hover || !out_pressed || !label) {
        return;
    }

    if (strcmp(label, "=") == 0) {
        *out_base = palette->redwood;
        *out_hover = palette->burnt_sienna;
        *out_pressed = palette->burnt_sienna;
        return;
    }

    if (label[0] && label[1] == '\0' && strchr("+-*/", label[0])) {
        *out_base = palette->burnt_sienna;
        *out_hover = palette->rosy_brown;
        *out_pressed = palette->redwood;
        return;
    }

    if (strcmp(label, "C") == 0 || strcmp(label, "+/-") == 0 || strcmp(label, "%") == 0) {
        *out_base = palette->champagne;
        *out_hover = palette->rosy_brown;
        *out_pressed = palette->burnt_sienna;
        return;
    }

    *out_base = palette->rosy_brown;
    *out_hover = palette->champagne;
    *out_pressed = palette->burnt_sienna;
}

static void calculator_style_keypad_row(ui_row_t *row, ui_color_t background)
{
    if (!row) {
        return;
    }
    ui_style_t style;
    ui_style_init(&style);
    style.background_color = background;
    style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR;
    ui_widget_set_style(ui_row_widget_mutable(row), &style);
}

int main(void)
{
    const ui_hal_ops_t *hal = ui_hal_test_sdl_ops();
    if (!hal) {
        return 1;
    }

    ui_scene_t *scene = ui_scene_create(hal);
    if (!scene) {
        return 1;
    }

    ui_column_t *root = ui_column_create();
    if (!root) {
        ui_scene_destroy(scene);
        return 1;
    }

    const int column_spacing = 6;
    const int headline_height = 18;
    const int display_height = 48;
    ui_column_set_spacing(root, column_spacing);
    ui_column_set_alignment(root, UI_MAIN_AXIS_START);
    ui_column_set_horizontal_alignment(root, UI_CROSS_AXIS_CENTER);
    ui_widget_set_bounds(ui_column_widget_mutable(root), 0, 0, UI_FRAMEBUFFER_WIDTH, UI_FRAMEBUFFER_HEIGHT);

    ui_style_t root_style;
    ui_style_init(&root_style);
    classic_noir_palette_t palette = {
        .seashell = ui_color_from_hex(CLASSIC_NOIR_PALETTE_HEX.seashell),
        .champagne = ui_color_from_hex(CLASSIC_NOIR_PALETTE_HEX.champagne),
        .rosy_brown = ui_color_from_hex(CLASSIC_NOIR_PALETTE_HEX.rosy_brown),
        .burnt_sienna = ui_color_from_hex(CLASSIC_NOIR_PALETTE_HEX.burnt_sienna),
        .redwood = ui_color_from_hex(CLASSIC_NOIR_PALETTE_HEX.redwood),
        .dark_text = ui_color_from_hex(CLASSIC_NOIR_PALETTE_HEX.dark_text)
    };
    root_style.flags = UI_STYLE_FLAG_BACKGROUND_COLOR;
    root_style.background_color = palette.seashell;
    root_style.padding_left = 12;
    root_style.padding_right = 12;
    root_style.padding_top = 12;
    root_style.padding_bottom = 12;
    ui_widget_set_style(ui_column_widget_mutable(root), &root_style);

    const int content_width = UI_FRAMEBUFFER_WIDTH - 32;
    const int button_spacing = 4;

    ui_text_t *headline = ui_text_create();
    ui_text_set_value(headline, "Classic Noir Calculator");
    ui_text_set_align(headline, UI_TEXT_ALIGN_CENTER);
    ui_text_set_color(headline, palette.redwood);
    ui_text_set_background_color(headline, palette.champagne);
    ui_widget_set_bounds(ui_text_widget_mutable(headline), 0, 0, content_width, headline_height);

    ui_text_t *display = ui_text_create();
    ui_text_set_align(display, UI_TEXT_ALIGN_RIGHT);
    ui_text_set_no_wrap(display, true);
    ui_text_set_max_lines(display, 1);
    ui_text_set_background_color(display, palette.champagne);
    ui_text_set_color(display, palette.dark_text);
    ui_widget_set_bounds(ui_text_widget_mutable(display), 0, 0, content_width, display_height);

    if (!headline || !display) {
        ui_text_destroy(display);
        ui_text_destroy(headline);
        ui_column_destroy(root);
        ui_scene_destroy(scene);
        return 1;
    }

    calculator_state_t calculator = {
        .display = display
    };
    calculator_reset(&calculator);

    ui_column_add_control(root, ui_text_widget_mutable(headline), false, "headline");
    ui_column_add_control(root, ui_text_widget_mutable(display), false, "display");

    enum {
        STANDARD_ROWS = 4,
        BUTTON_COLUMNS = 4,
        BOTTOM_BUTTONS = 3,
        KEYPAD_ROWS = STANDARD_ROWS + 1,
        TOTAL_BUTTON_COUNT = STANDARD_ROWS * BUTTON_COLUMNS + BOTTOM_BUTTONS
    };

    static const char *button_layout[STANDARD_ROWS][BUTTON_COLUMNS] = {
        {"C", "+/-", "%", "/"},
        {"7", "8", "9", "*"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"}
    };

    static const char *bottom_row[BOTTOM_BUTTONS] = {"0", ".", "="};

    ui_row_t *rows[KEYPAD_ROWS] = {0};
    ui_button_t *buttons[TOTAL_BUTTON_COUNT] = {0};
    calculator_button_ctx_t button_contexts[TOTAL_BUTTON_COUNT] = {0};

    const int total_controls = 2 + KEYPAD_ROWS;
    const int total_spacing = column_spacing * (total_controls - 1);
    const int padding_vertical = root_style.padding_top + root_style.padding_bottom;
    int available_keypad_height = UI_FRAMEBUFFER_HEIGHT - padding_vertical - headline_height -
                                  display_height - total_spacing;
    if (available_keypad_height < KEYPAD_ROWS) {
        available_keypad_height = KEYPAD_ROWS;
    }
    int row_height = available_keypad_height / KEYPAD_ROWS;
    if (row_height <= 0) {
        row_height = 1;
    }
    int extra_height = available_keypad_height - row_height * KEYPAD_ROWS;

    const int button_width = (content_width - button_spacing * (BUTTON_COLUMNS - 1)) / BUTTON_COLUMNS;
    size_t row_index = 0;
    size_t button_index = 0;
    bool build_success = true;

    for (size_t r = 0; r < STANDARD_ROWS; ++r) {
        int current_row_height = row_height;
        ui_row_t *row = ui_row_create();
        if (!row) {
            build_success = false;
            break;
        }
        rows[row_index++] = row;
        ui_row_set_spacing(row, button_spacing);
        ui_row_set_run_spacing(row, button_spacing);
        ui_row_set_alignment(row, UI_MAIN_AXIS_CENTER);
        ui_row_set_vertical_alignment(row, UI_CROSS_AXIS_CENTER);
        ui_widget_set_bounds(ui_row_widget_mutable(row), 0, 0, content_width, current_row_height);
        calculator_style_keypad_row(row, palette.seashell);

        for (size_t c = 0; c < BUTTON_COLUMNS; ++c) {
            const char *label = button_layout[r][c];
            ui_button_t *button = ui_button_create();
            if (!button) {
                build_success = false;
                break;
            }
            buttons[button_index] = button;
            button_contexts[button_index].state = &calculator;
            button_contexts[button_index].label = label;

            ui_button_set_text(button, label);

            ui_color_t base = palette.rosy_brown;
            ui_color_t hover = palette.champagne;
            ui_color_t pressed = palette.burnt_sienna;
            calculator_pick_button_colors(&palette, label, &base, &hover, &pressed);
            calculator_configure_button(button, base, hover, pressed, palette.dark_text, palette.champagne);
            ui_button_set_font(button, NULL);
            ui_widget_set_bounds(ui_button_widget_mutable(button), 0, 0, button_width, current_row_height);
            ui_button_set_on_click(button, calculator_on_button, &button_contexts[button_index]);

            if (!ui_row_add_control(row, ui_button_widget_mutable(button), false, label)) {
                build_success = false;
                break;
            }

            ++button_index;
        }

        if (!build_success) {
            break;
        }

        ui_column_add_control(root, ui_row_widget_mutable(row), false, "keypad-row");
    }

    if (build_success) {
        ui_row_t *final_row = ui_row_create();
        if (!final_row) {
            build_success = false;
        } else {
            rows[row_index++] = final_row;
            ui_row_set_spacing(final_row, button_spacing);
            ui_row_set_run_spacing(final_row, button_spacing);
            ui_row_set_alignment(final_row, UI_MAIN_AXIS_CENTER);
            ui_row_set_vertical_alignment(final_row, UI_CROSS_AXIS_CENTER);
            int final_row_height = row_height + extra_height;
            ui_widget_set_bounds(ui_row_widget_mutable(final_row), 0, 0, content_width, final_row_height);
            calculator_style_keypad_row(final_row, palette.seashell);

            const int zero_width = button_width * 2 + button_spacing;

            for (size_t c = 0; c < BOTTOM_BUTTONS; ++c) {
                const char *label = bottom_row[c];
                ui_button_t *button = ui_button_create();
                if (!button) {
                    build_success = false;
                    break;
                }
                buttons[button_index] = button;
                button_contexts[button_index].state = &calculator;
                button_contexts[button_index].label = label;

                ui_button_set_text(button, label);

                ui_color_t base = palette.rosy_brown;
                ui_color_t hover = palette.champagne;
                ui_color_t pressed = palette.burnt_sienna;
                calculator_pick_button_colors(&palette, label, &base, &hover, &pressed);
                calculator_configure_button(button, base, hover, pressed, palette.dark_text, palette.champagne);
                ui_button_set_font(button, NULL);
                int width = (strcmp(label, "0") == 0) ? zero_width : button_width;
                ui_widget_set_bounds(ui_button_widget_mutable(button), 0, 0, width, final_row_height);
                ui_button_set_on_click(button, calculator_on_button, &button_contexts[button_index]);

                if (!ui_row_add_control(final_row, ui_button_widget_mutable(button), false, label)) {
                    build_success = false;
                    break;
                }

                ++button_index;
            }

            if (build_success) {
                ui_column_add_control(root, ui_row_widget_mutable(final_row), false, "keypad-row");
            }
        }
    }

    if (!build_success) {
        for (size_t i = 0; i < button_index; ++i) {
            ui_button_destroy(buttons[i]);
        }
        for (size_t i = 0; i < row_index; ++i) {
            ui_row_destroy(rows[i]);
        }
        ui_text_destroy(display);
        ui_text_destroy(headline);
        ui_column_destroy(root);
        ui_scene_destroy(scene);
        return 1;
    }

    ui_scene_set_root(scene, ui_column_widget_mutable(root));
    ui_scene_run(scene);

    for (size_t i = 0; i < button_index; ++i) {
        ui_button_destroy(buttons[i]);
    }
    for (size_t i = 0; i < row_index; ++i) {
        ui_row_destroy(rows[i]);
    }
    ui_text_destroy(display);
    ui_text_destroy(headline);
    ui_column_destroy(root);
    ui_scene_destroy(scene);
    return 0;
}
