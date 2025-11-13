#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

typedef enum {
    UI_MAIN_AXIS_START,
    UI_MAIN_AXIS_END,
    UI_MAIN_AXIS_CENTER,
    UI_MAIN_AXIS_SPACE_BETWEEN,
    UI_MAIN_AXIS_SPACE_AROUND,
    UI_MAIN_AXIS_SPACE_EVENLY
} ui_main_axis_alignment_t;

typedef enum {
    UI_CROSS_AXIS_START,
    UI_CROSS_AXIS_END,
    UI_CROSS_AXIS_CENTER,
    UI_CROSS_AXIS_STRETCH
} ui_cross_axis_alignment_t;

#endif
