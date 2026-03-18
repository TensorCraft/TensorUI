#include "Switch.h"
#include "../../hal/math/math.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"

static Color switchGetPixel(void *self, int x, int y) {
    Switch *sw = (Switch *)self;
    int w = sw->width;
    int h = sw->height;
    int r = h / 2;
    
    // Pill Track with simple AA
    float distPill = -1;
    if (x < r) distPill = hal_sqrtf(((r-x)*(r-x)) + ((r-y)*(r-y)));
    else if (x >= w-r) distPill = hal_sqrtf(((x-(w-r-1))*(x-(w-r-1))) + ((r-y)*(r-y)));
    else if (y >= 0 && y < h) distPill = 0;

    if (distPill > r) return COLOR_TRANSPARENT;

    // Thumb with simple AA
    int thumbR = sw->isOn ? (r - 2) : (r - 6);
    int thumbX = sw->isOn ? (w - r) : r;
    float distThumb = hal_sqrtf(((x - thumbX)*(x - thumbX)) + ((y - r)*(y - r)));
    
    if (distThumb <= thumbR) return sw->thumbColor;
    if (distThumb <= thumbR + 1) {
        Color c = sw->isOn ? sw->onColor : sw->offColor;
        // Blend thumb with background
        float ratio = distThumb - thumbR;
        Color res;
        res.r = sw->thumbColor.r * (1.0 - ratio) + c.r * ratio;
        res.g = sw->thumbColor.g * (1.0 - ratio) + c.g * ratio;
        res.b = sw->thumbColor.b * (1.0 - ratio) + c.b * ratio;
        res.transparent = false;
        return res;
    }

    if (distPill > r - 1) {
        Color c = sw->isOn ? sw->onColor : sw->offColor;
        float ratio = distPill - (r - 1);
        return (Color){c.r * (1.0 - ratio), c.g * (1.0 - ratio), c.b * (1.0 - ratio), false};
    }

    return sw->isOn ? sw->onColor : sw->offColor;
}

static void switchOnClick(void *self) {
    Switch *sw = (Switch *)self;
    sw->isOn = !sw->isOn;
    if (sw->onToggle) sw->onToggle(sw->isOn);
    renderFlag = true;
}

Switch* createSwitch(int x, int y, bool initialState, void (*onToggle)(bool state)) {
    Switch *sw = (Switch *)hal_malloc(sizeof(Switch));
    sw->width = 45;
    sw->height = 24;
    sw->x = x;
    sw->y = y;
    sw->isOn = initialState;
    sw->onColor = (Color){103, 80, 164, false}; // Material M3 Deep Purple
    sw->offColor = (Color){73, 69, 79, false};  // Material M3 Gray
    sw->thumbColor = COLOR_WHITE;
    sw->onToggle = onToggle;

    sw->frameId = requestFrame(sw->width, sw->height, x, y, sw, NULL, switchGetPixel, switchOnClick, NULL);
    return sw;
}
