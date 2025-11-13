#ifndef UI_CORE_H
#define UI_CORE_H

#include "ui_primitives.h"

/*
 * ui_core.h now represents the public entry point for the UI stack.
 * ui_primitives.h exposes the framebuffer + HAL primitives, so any higher-
 * level widget layer can sit on top of it without leaking implementation.
 * Additional widget helpers, layout managers, and controls will appear below
 * in future iterations.
 */

#endif
