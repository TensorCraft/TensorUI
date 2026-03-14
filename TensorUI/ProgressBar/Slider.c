#include "Slider.h"
#include <stdlib.h>
#include "../../hal/screen/screen.h"

extern Frame Frames[];

static Color sliderGetPixel(void *self, int x, int y) {
    Slider *sl = (Slider *)self;
    int r = sl->height / 2;
    int thumbR = r + 2;
    int thumbX = (int)(r + (sl->width - 2 * r) * sl->value);

    // Thumb check
    if ((x - thumbX) * (x - thumbX) + (y - r) * (y - r) <= thumbR * thumbR) {
        return sl->thumbColor;
    }

    // Bar check
    if (y >= r - 2 && y <= r + 2) { // 4px thin bar
        if (x >= r && x <= sl->width - r) {
            if (x <= thumbX) return sl->barColor;
            return sl->bgColor;
        }
    }

    return COLOR_TRANSPARENT;
}

static void sliderOnTouch(void *self, bool isDown) {
    Slider *sl = (Slider *)self;
    sl->isDragging = isDown;
    
    if (isDown) {
        // We need the current mouse position to update value immediately on touch
        int mx, my;
        getTouchState(&mx, &my);
        int localX = mx - Frames[sl->frameId].x;
        int r = sl->height / 2;
        float newVal = (float)(localX - r) / (sl->width - 2 * r);
        if (newVal < 0) newVal = 0;
        if (newVal > 1) newVal = 1;
        if (sl->value != newVal) {
            sl->value = newVal;
            if (sl->onChange) sl->onChange(sl->value);
        }
    }
}

static void sliderPreRender(void *self) {
    Slider *sl = (Slider *)self;
    if (sl->isDragging) {
        int mx, my;
        getTouchState(&mx, &my);
        int localX = mx - Frames[sl->frameId].x;
        int r = sl->height / 2;
        float newVal = (float)(localX - r) / (sl->width - 2 * r);
        if (newVal < 0) newVal = 0;
        if (newVal > 1) newVal = 1;
        if (sl->value != newVal) {
            sl->value = newVal;
            if (sl->onChange) sl->onChange(sl->value);
        }
    }
}

Slider* createSlider(int x, int y, int w, int h, float initialValue, void (*onChange)(float value)) {
    Slider *sl = (Slider *)malloc(sizeof(Slider));
    sl->x = x;
    sl->y = y;
    sl->width = w;
    sl->height = h;
    sl->value = initialValue;
    sl->barColor = (Color){0, 122, 255, false};
    sl->bgColor = (Color){100, 100, 100, false};
    sl->thumbColor = COLOR_WHITE;
    sl->onChange = onChange;
    sl->isDragging = false;

    int frameId = requestFrame(w, h, x, y, sl, sliderPreRender, sliderGetPixel, NULL, sliderOnTouch);
    sl->frameId = frameId;
    Frames[frameId].scrollType = 2; // Register as a horizontal drag target
    return sl;
}
