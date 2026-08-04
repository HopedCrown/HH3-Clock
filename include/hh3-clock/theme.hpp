#include <gint/display.h>

typedef struct THEME {
    int fg;
    int bg;
    int lineColor;
    int textColor;
    int clock_fill;
    int clock_border;
    int warnColor;
    int errorColor;
    int TitleColor;
    int titlebgColor;
} THEME;

THEME W_B_Theme = {C_BLACK,C_WHITE,C_BLACK,C_BLACK,C_WHITE,C_BLACK,C_LIGHT,C_BLACK,C_WHITE,C_BLACK};