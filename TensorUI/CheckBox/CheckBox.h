#ifndef CHECKBOX_H
#define CHECKBOX_H

#include "../Color/color.h"
#include <stdbool.h>

typedef struct {
    int x, y, width, height;
    bool isChecked;
    Color boxColor;
    Color checkColor;
    Color tickColor;
    void (*onToggle)(bool checked);
    int frameId;
} CheckBox;

CheckBox* createCheckBox(int x, int y, bool initialState, void (*onToggle)(bool));

#endif
