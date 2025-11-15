#ifndef UI_BORDER_RADIUS_H
#define UI_BORDER_RADIUS_H

typedef struct {
    int top_left;
    int top_right;
    int bottom_right;
    int bottom_left;
} ui_border_radius_t;

static inline ui_border_radius_t ui_border_radius_zero(void)
{
    return (ui_border_radius_t){0, 0, 0, 0};
}

static inline ui_border_radius_t ui_border_radius_all(int radius)
{
    return (ui_border_radius_t){radius, radius, radius, radius};
}

#endif
