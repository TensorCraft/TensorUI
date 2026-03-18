#ifndef THEME_H
#define THEME_H

#include "../Color/color.h"

typedef struct {
    Color primary;
    Color onPrimary;
    Color secondary;
    Color onSecondary;
    Color surface;
    Color onSurface;
    Color surfaceVariant;
    Color outline;
    Color error;
    Color background;
} UITheme;

extern UITheme CurrentTheme;

void initDefaultTheme();
void applyTheme(UITheme newTheme);

#endif
