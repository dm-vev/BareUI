#ifndef UI_FONT_H
#define UI_FONT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BAREUI_FONT_HEIGHT 8
#define BAREUI_FONT_MAX_WIDTH 8
/* extra blank columns after each glyph when drawing */
#ifndef BAREUI_FONT_SPACING
#define BAREUI_FONT_SPACING 2
#endif

/* shadow copy of the generated entry so we can compile the data easily */
typedef struct {
    uint32_t codepoint;
    uint8_t width;
    uint8_t columns[BAREUI_FONT_MAX_WIDTH];
} bareui_font_entry_t;

typedef struct {
    const bareui_font_entry_t *entries;
    size_t count;
    uint8_t height;
} bareui_font_t;

typedef struct {
    uint32_t codepoint;
    uint8_t width;
    uint8_t height;
    const uint8_t *columns;
    uint8_t spacing;
} bareui_font_glyph_t;

const bareui_font_t *bareui_font_default(void);
bool bareui_font_lookup(const bareui_font_t *font, uint32_t codepoint,
                        bareui_font_glyph_t *glyph_out);

/* Built-in low-resolution ASCII font (5x7), tuned for readability */
const bareui_font_t *bareui_font_lc_lores_ascii(void);

#endif
