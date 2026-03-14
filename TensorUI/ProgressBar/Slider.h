#ifndef SLIDER_H
#define SLIDER_H

#include "../Color/color.h"

typedef struct {
    int x, y, width, height;
    float value; // 0.0 to 1.0
    Color barColor;
    Color bgColor;
    Color thumbColor;
    void (*onChange)(float value);
    bool isDragging;
    int frameId;
} Slider;

Slider* createSlider(int x, int y, int w, int h, float initialValue, void (*onChange)(float value));

#endif
