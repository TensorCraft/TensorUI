#ifndef RADIOBUTTON_H
#define RADIOBUTTON_H

#include "../Color/color.h"
#include <stdbool.h>

typedef struct {
    int x, y, radius;
    int *selection;
    int value;
    Color outerColor;
    Color innerColor;
    void (*onSelect)(void* arg);
    void* arg;
    int frameId;
} RadioButton;

RadioButton* createRadioButton(int x, int y, int *selection, int value, void (*onSelect)(void*), void* arg);


#endif
