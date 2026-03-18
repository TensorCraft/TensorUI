#include "Theme.h"
#include <stdbool.h>

extern bool renderFlag;

UITheme CurrentTheme;


void initDefaultTheme() {
    CurrentTheme.primary = M3_PRIMARY;
    CurrentTheme.onPrimary = M3_ON_PRIMARY;
    CurrentTheme.secondary = M3_SECONDARY;
    CurrentTheme.onSecondary = (Color){255, 255, 255, false};
    CurrentTheme.surface = M3_SURFACE;
    CurrentTheme.onSurface = M3_ON_SURFACE;
    CurrentTheme.surfaceVariant = M3_SURFACE_VARIANT;
    CurrentTheme.outline = M3_OUTLINE;
    CurrentTheme.error = M3_ERROR;
    CurrentTheme.background = M3_DARK_BG;
}

void applyTheme(UITheme newTheme) {
    CurrentTheme = newTheme;
    renderFlag = true;
}
