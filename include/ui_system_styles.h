#ifndef UI_SYSTEM_STYLES_H
#define UI_SYSTEM_STYLES_H

#include "ui_widget.h"

#include <stdint.h>

typedef enum {
    UI_SYSTEM_STYLE_AMETHYST = 0,
    UI_SYSTEM_STYLE_VERDANT,
    UI_SYSTEM_STYLE_DAWN,
    UI_SYSTEM_STYLE_CLASSIC,
    UI_SYSTEM_STYLE_COUNT
} ui_system_style_t;

const char *ui_system_style_name(ui_system_style_t style);
void ui_system_style_fill(ui_style_t *out, ui_system_style_t style);

#endif
