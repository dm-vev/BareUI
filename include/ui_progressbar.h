#ifndef UI_PROGRESSBAR_H
#define UI_PROGRESSBAR_H

#include "ui_widget.h"
#include "ui_border_radius.h"

#include <stdbool.h>

typedef struct ui_progressbar ui_progressbar_t;

ui_progressbar_t *ui_progressbar_create(void);
void ui_progressbar_destroy(ui_progressbar_t *progressbar);
void ui_progressbar_init(ui_progressbar_t *progressbar);

void ui_progressbar_set_value(ui_progressbar_t *progressbar, double value);
double ui_progressbar_value(const ui_progressbar_t *progressbar);
void ui_progressbar_clear_value(ui_progressbar_t *progressbar);
bool ui_progressbar_is_determinate(const ui_progressbar_t *progressbar);

void ui_progressbar_set_bar_height(ui_progressbar_t *progressbar, int height);
int ui_progressbar_bar_height(const ui_progressbar_t *progressbar);

void ui_progressbar_set_border_radius(ui_progressbar_t *progressbar,
                                      ui_border_radius_t radius);
ui_border_radius_t ui_progressbar_border_radius(const ui_progressbar_t *progressbar);

void ui_progressbar_set_semantics_label(ui_progressbar_t *progressbar, const char *label);
const char *ui_progressbar_semantics_label(const ui_progressbar_t *progressbar);

void ui_progressbar_set_semantics_value(ui_progressbar_t *progressbar, const char *value);
const char *ui_progressbar_semantics_value(const ui_progressbar_t *progressbar);

void ui_progressbar_set_tooltip(ui_progressbar_t *progressbar, const char *tooltip);
const char *ui_progressbar_tooltip(const ui_progressbar_t *progressbar);

const ui_widget_t *ui_progressbar_widget(const ui_progressbar_t *progressbar);
ui_widget_t *ui_progressbar_widget_mutable(ui_progressbar_t *progressbar);

#endif
