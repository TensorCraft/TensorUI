#ifndef SWITCH_H
#define SWITCH_H

#include "../Color/color.h"
#include <stdbool.h>

typedef struct {
    int x, y, width, height;
    bool isOn;
    Color onColor;
    Color offColor;
    Color thumbColor;
    void (*onToggle)(bool state);
    int frameId;
} Switch;

Switch* createSwitch(int x, int y, bool initialState, void (*onToggle)(bool state));

#endif
