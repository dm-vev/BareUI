#include "ui_font.h"
#include "font/bareui_font_lores5x7.h"

static bareui_font_entry_t lores_entries[95];
static bareui_font_t lores_font;

const bareui_font_t *bareui_font_lc_lores_ascii(void)
{
    static int inited = 0;
    if (!inited) {
        for (int i = 0; i < 95; ++i) {
            lores_entries[i].codepoint = (uint32_t)(32 + i);
            lores_entries[i].width = 5;
            for (int c = 0; c < 5; ++c) {
                lores_entries[i].columns[c] = bareui_lores5x7_table[i][c];
            }
            for (int c = 5; c < 8; ++c) {
                lores_entries[i].columns[c] = 0;
            }
        }
        lores_font.entries = lores_entries;
        lores_font.count = 95;
        lores_font.height = BAREUI_FONT_HEIGHT;
        inited = 1;
    }
    return &lores_font;
}
