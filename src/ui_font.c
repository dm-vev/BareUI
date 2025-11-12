#include "ui_font.h"
#include "font/bareui_font_data.h"
/* lo-res 5x7 table exists, but is disabled by default for now. */

/* By default, prefer the crisp low-res ASCII; fall back to generated table. */
extern const bareui_font_t *bareui_font_lc_lores_ascii(void);
static const bareui_font_t default_font = {
    .entries = NULL,
    .count = 0,
    .height = BAREUI_FONT_HEIGHT
};

const bareui_font_t *bareui_font_default(void)
{
    return &default_font;
}

bool bareui_font_lookup(const bareui_font_t *font, uint32_t codepoint,
                        bareui_font_glyph_t *glyph_out)
{
    if (!font || !glyph_out) {
        return false;
    }

    /* No special-case ASCII; rely on generated coverage for correctness. */

    for (size_t i = 0; i < font->count; ++i) {
        const bareui_font_entry_t *entry = &font->entries[i];
        if (entry->codepoint == codepoint) {
            glyph_out->codepoint = entry->codepoint;
            glyph_out->width = entry->width;
            glyph_out->height = font->height;
            glyph_out->columns = entry->columns;
            glyph_out->spacing = (uint8_t)(entry->width + BAREUI_FONT_SPACING);
            return true;
        }
    }
    /* Fallback to generated font with extended coverage (e.g., Cyrillic) */
    for (size_t i = 0; i < BAREUI_FONT_ENTRY_COUNT; ++i) {
        if (bareui_font_data[i].codepoint == codepoint) {
            glyph_out->codepoint = bareui_font_data[i].codepoint;
            glyph_out->width = bareui_font_data[i].width;
            glyph_out->height = font->height;
            glyph_out->columns = bareui_font_data[i].columns;
            glyph_out->spacing = (uint8_t)(bareui_font_data[i].width + BAREUI_FONT_SPACING);
            return true;
        }
    }
    return false;
}
