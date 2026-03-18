#include "Toggle.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"

static Color toggleGetPixel(void *self, int x, int y) {
    Toggle *tg = (Toggle *)self;
    int r = tg->height / 2;
    
    // Check pill shape
    bool inPill = false;
    if (x < r) { // Left semi-circle
        if ((x - r) * (x - r) + (y - r) * (y - r) <= r * r) inPill = true;
    } else if (x >= tg->width - r) { // Right semi-circle
        if ((x - (tg->width - r - 1)) * (x - (tg->width - r - 1)) + (y - r) * (y - r) <= r * r) inPill = true;
    } else { // Middle rect
        inPill = true;
    }

    if (!inPill) return COLOR_TRANSPARENT;

    // Thumb logic
    int thumbR = r - 2;
    int thumbX = tg->isOn ? (tg->width - r) : r;
    if ((x - thumbX) * (x - thumbX) + (y - r) * (y - r) <= thumbR * thumbR) {
        return tg->thumbColor;
    }

    return tg->isOn ? tg->onColor : tg->offColor;
}

static void toggleOnClick(void *self) {
    Toggle *tg = (Toggle *)self;
    tg->isOn = !tg->isOn;
    if (tg->onToggle) {
        tg->onToggle(tg->isOn);
    }
}

Toggle* createToggle(int x, int y, bool initialState, void (*onToggle)(bool state)) {
    Toggle *tg = (Toggle *)hal_malloc(sizeof(Toggle));
    tg->x = x;
    tg->y = y;
    tg->width = 50;  // Default size
    tg->height = 26;
    tg->isOn = initialState;
    tg->onColor = (Color){48, 209, 88, false}; // iOS Green
    tg->offColor = (Color){120, 120, 128, false}; // iOS Gray
    tg->thumbColor = COLOR_WHITE;
    tg->onToggle = onToggle;

    tg->frameId = requestFrame(tg->width, tg->height, x, y, tg, NULL, toggleGetPixel, toggleOnClick, NULL);
    if (tg->frameId == -1) {
        hal_free(tg);
        return NULL;
    }
    return tg;
}
