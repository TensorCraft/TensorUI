#include "CheckBox.h"
#include "../../hal/math/math.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"

static Color checkboxGetPixel(void *self, int x, int y) {
    CheckBox *cb = (CheckBox *)self;
    int w = cb->width;
    int h = cb->height;
    int r = 4;
    
    // Rounded square logic with simple AA
    float dist = -1;
    bool inCorner = false;
    if (x < r && y < r) { dist = hal_sqrtf(((r-x)*(r-x)) + ((r-y)*(r-y))); inCorner = true; }
    else if (x >= w-r && y < r) { dist = hal_sqrtf(((x-(w-r-1))*(x-(w-r-1))) + ((r-y)*(r-y))); inCorner = true; }
    else if (x < r && y >= h-r) { dist = hal_sqrtf(((r-x)*(r-x)) + ((y-(h-r-1))*(y-(h-r-1)))); inCorner = true; }
    else if (x >= w-r && y >= h-r) { dist = hal_sqrtf(((x-(w-r-1))*(x-(w-r-1))) + ((y-(h-r-1))*(y-(h-r-1)))); inCorner = true; }

    if (inCorner && dist > r) return COLOR_TRANSPARENT;

    if (cb->isChecked) {
        // Filled state
        Color boxCol = cb->checkColor;
        
        // Edge smoothing
        if (inCorner && dist > r - 1) {
            float ratio = dist - (r - 1);
            return (Color){boxCol.r * (1.0-ratio), boxCol.g * (1.0-ratio), boxCol.b * (1.0-ratio), false};
        }

        // Draw tick with subtle Alpha
        bool inTick = false;
        // Segment 1: (5, 10) to (9, 14)
        if (x >= 5 && x <= 9 && abs(y - (x + 5)) <= 1) inTick = true;
        // Segment 2: (9, 14) to (15, 8)
        if (x > 9 && x <= 15 && abs(y - (-x + 23)) <= 1) inTick = true;
        
        if (inTick) return cb->tickColor;
        return boxCol;
    }

    // Border state
    bool isBorder = (x <= 1 || x >= w - 2 || y <= 1 || y >= h - 2);
    if (isBorder) {
        if (inCorner && dist > r - 1) {
             float ratio = dist - (r - 1);
             return (Color){cb->boxColor.r * (1.0-ratio), cb->boxColor.g * (1.0-ratio), cb->boxColor.b * (1.0-ratio), false};
        }
        return cb->boxColor;
    }
    
    return COLOR_TRANSPARENT;
}

static void checkboxOnClick(void *self) {
    CheckBox *cb = (CheckBox *)self;
    cb->isChecked = !cb->isChecked;
    if (cb->onToggle) {
        cb->onToggle(cb->isChecked);
    }
    renderFlag = true;
}

CheckBox* createCheckBox(int x, int y, bool initialState, void (*onToggle)(bool)) {
    CheckBox *cb = (CheckBox *)hal_malloc(sizeof(CheckBox));
    cb->x = x;
    cb->y = y;
    cb->width = 22;
    cb->height = 22;
    cb->isChecked = initialState;
    cb->boxColor = (Color){150, 150, 150, false};
    cb->checkColor = (Color){33, 150, 243, false}; // Material Blue
    cb->tickColor = COLOR_WHITE;
    cb->onToggle = onToggle;

    cb->frameId = requestFrame(cb->width, cb->height, x, y, cb, NULL, checkboxGetPixel, checkboxOnClick, NULL);
    return cb;
}
