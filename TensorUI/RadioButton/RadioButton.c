#include "RadioButton.h"
#include "../../hal/math/math.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"

static Color radioGetPixel(void *self, int x, int y) {
    RadioButton *rb = (RadioButton *)self;
    int r = rb->radius;
    int centerX = r;
    int centerY = r;
    
    float dist = hal_sqrtf(((x - centerX)*(x - centerX)) + ((y - centerY)*(y - centerY)));
    if (dist > r) return COLOR_TRANSPARENT;
    
    bool isSelected = (rb->selection != NULL && *(rb->selection) == rb->value);

    // Inner dot with AA
    if (isSelected) {
        int innerR = r - 5;
        if (dist <= innerR) return rb->innerColor;
        if (dist <= innerR + 1) {
            float ratio = dist - innerR;
            // High perf: assume background is transparent or dark
            return (Color){rb->innerColor.r * (1.0-ratio), rb->innerColor.g * (1.0-ratio), rb->innerColor.b * (1.0-ratio), false};
        }
    }
    
    // Outer border with AA
    if (dist > r - 2) {
        float ratio = dist - (r - 2);
        if (ratio > 1) ratio = 1;
        // Edge fade
        float fade = (dist > r - 1) ? (1.0 - (dist - (r - 1))) : 1.0;
        return (Color){rb->outerColor.r * fade, rb->outerColor.g * fade, rb->outerColor.b * fade, false};
    }
    
    return COLOR_TRANSPARENT;
}

static void radioOnClick(void *self) {
    RadioButton *rb = (RadioButton *)self;
    if (rb->selection != NULL && *(rb->selection) != rb->value) {
        *(rb->selection) = rb->value;
        if (rb->onSelect) {
            rb->onSelect(rb->arg);
        }
    }
    renderFlag = true;
}

RadioButton* createRadioButton(int x, int y, int *selection, int value, void (*onSelect)(void*), void* arg) {
    RadioButton *rb = (RadioButton *)hal_malloc(sizeof(RadioButton));
    rb->radius = 11;
    rb->x = x;
    rb->y = y;
    rb->selection = selection;
    rb->value = value;
    rb->outerColor = (Color){150, 150, 150, false};
    rb->innerColor = (Color){25, 118, 210, false}; // Material Blue 700
    rb->onSelect = onSelect;
    rb->arg = arg;

    rb->frameId = requestFrame(rb->radius * 2, rb->radius * 2, x, y, rb, NULL, radioGetPixel, radioOnClick, NULL);
    return rb;
}
