#include "ui_system_styles.h"

typedef struct {
    const char *name;
    uint32_t background;
    uint32_t foreground;
    uint32_t accent;
    uint32_t border;
    uint32_t shadow;
    int shadow_offset_x;
    int shadow_offset_y;
    int shadow_blur;
} ui_system_style_entry_t;

static const ui_system_style_entry_t style_table[UI_SYSTEM_STYLE_COUNT] = {
    {
        .name = "Amethyst Bloom",
        .background = 0xe5d4ed, // Pale purple
        .foreground = 0x514f59, // Davy's gray
        .accent = 0x5941a9,     // Rebecca purple
        .border = 0x6d72c3,     // Medium slate blue
        .shadow = 0x1d1128,     // Dark purple
        .shadow_offset_x = 0,
        .shadow_offset_y = 2,
        .shadow_blur = 5
    },
    {
        .name = "Verdant Vale",
        .background = 0xb5cbb7, // Ash gray
        .foreground = 0x818479, // Battleship gray
        .accent = 0xd2e4c4,     // Tea green
        .border = 0xe4e9b2,     // Cream
        .shadow = 0xe7e08b,     // Flax
        .shadow_offset_x = 0,
        .shadow_offset_y = 1,
        .shadow_blur = 4
    },
    {
        .name = "Dawn Sand",
        .background = 0xf5f1ed, // Isabelline
        .foreground = 0x252323, // Raisin black
        .accent = 0xc97c5d,     // Burnt sienna
        .border = 0xa99985,     // Khaki
        .shadow = 0x70798c,     // Slate gray
        .shadow_offset_x = 0,
        .shadow_offset_y = 3,
        .shadow_blur = 6
    },
    {
        .name = "Classic Noir",
        .background = 0xEEE2DF, // Seashell
        .foreground = 0x1A1A1A, // Near black
        .accent = 0xC97C5D,     // Burnt sienna
        .border = 0xB36A5E,     // Redwood
        .shadow = 0xEED7C5,     // Champagne pink
        .shadow_offset_x = 0,
        .shadow_offset_y = 1,
        .shadow_blur = 0
    }
};

const char *ui_system_style_name(ui_system_style_t style)
{
    if (style < 0 || style >= UI_SYSTEM_STYLE_COUNT) {
        return style_table[0].name;
    }
    return style_table[style].name;
}

void ui_system_style_fill(ui_style_t *out, ui_system_style_t style_id)
{
    if (!out) {
        return;
    }
    if (style_id < 0 || style_id >= UI_SYSTEM_STYLE_COUNT) {
        style_id = UI_SYSTEM_STYLE_AMETHYST;
    }
    const ui_system_style_entry_t *entry = &style_table[style_id];

    ui_style_init(out);
    out->background_color = ui_color_from_hex(entry->background);
    out->foreground_color = ui_color_from_hex(entry->foreground);
    out->accent_color = ui_color_from_hex(entry->accent);
    out->border_color = ui_color_from_hex(entry->border);
    out->border_width = 2;
    out->border_sides = UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM;
    out->box_shadow.enabled = true;
    out->box_shadow.offset_x = entry->shadow_offset_x;
    out->box_shadow.offset_y = entry->shadow_offset_y;
    out->box_shadow.blur_radius = entry->shadow_blur;
    out->box_shadow.spread_radius = 0;
    out->box_shadow.blur_style = UI_SHADOW_BLUR_NORMAL;
    out->box_shadow.color = ui_color_from_hex(entry->shadow);

    out->flags = UI_STYLE_FLAG_BACKGROUND_COLOR | UI_STYLE_FLAG_FOREGROUND_COLOR |
                 UI_STYLE_FLAG_ACCENT_COLOR | UI_STYLE_FLAG_BORDER_COLOR |
                 UI_STYLE_FLAG_BORDER_WIDTH | UI_STYLE_FLAG_BORDER_SIDES |
                 UI_STYLE_FLAG_BOX_SHADOW;
}
