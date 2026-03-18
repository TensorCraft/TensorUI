#include "Slider.h"
#include "../../hal/math/math.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"


extern Frame Frames[];

static Color sliderGetPixel(void *self, int x, int y) {
    Slider *sl = (Slider *)self;
    int r = sl->height / 2;
    int thumbR = r - 2; // Better proportion
    int thumbX = (int)(r + (sl->width - 2 * r) * sl->value);

    // Thumb check with simple AA
    float distThumb = hal_sqrtf(((x - thumbX)*(x - thumbX)) + ((y - r)*(y - r)));
    if (distThumb <= thumbR) return sl->thumbColor;
    if (distThumb <= thumbR + 1) {
        float ratio = distThumb - thumbR;
        return (Color){sl->thumbColor.r * (1.0-ratio), sl->thumbColor.g * (1.0-ratio), sl->thumbColor.b * (1.0-ratio), false};
    }

    // Bar check
    if (y >= r - 2 && y <= r + 2) { // 4px thin bar
        if (x >= r && x <= sl->width - r) {
            Color barCol = (x <= thumbX) ? sl->barColor : sl->bgColor;
            // Rounded bar ends? Simple check
            if (x == r || x == sl->width - r) return (Color){barCol.r/2, barCol.g/2, barCol.b/2, false};
            return barCol;
        }
    }

    return COLOR_TRANSPARENT;
}


static void sliderOnTouch(void *self, bool isDown) {
    Slider *sl = (Slider *)self;
    sl->isDragging = isDown;
    Frames[sl->frameId].continuousRender = isDown;
    
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
            renderFlag = true;
        }

    }
    renderFlag = true;
}

static void sliderCancelInteraction(void *self) {
    Slider *sl = (Slider *)self;
    sl->isDragging = false;
    Frames[sl->frameId].continuousRender = false;
    renderFlag = true;
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
            renderFlag = true;
        }

    }
}

Slider* createSlider(int x, int y, int w, int h, float initialValue, void (*onChange)(float value)) {
    Slider *sl = (Slider *)hal_malloc(sizeof(Slider));
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
    Frames[frameId].onCancelInteraction = sliderCancelInteraction;
    return sl;
}
