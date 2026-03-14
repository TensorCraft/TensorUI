#ifndef TOGGLE_H
#define TOGGLE_H

#include "../Color/color.h"
#include <stdbool.h>

typedef struct {
    int x, y, width, height;
    bool isOn;
    Color onColor;
    Color offColor;
    Color thumbColor;
    void (*onToggle)(bool state);
} Toggle;

Toggle* createToggle(int x, int y, bool initialState, void (*onToggle)(bool state));

#endif
