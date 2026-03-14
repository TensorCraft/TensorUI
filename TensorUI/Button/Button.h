#ifndef BUTTON_H
#define BUTTON_H

#include "../Label/Label.h"
#include "../Color/color.h"
#include "../Font/font.h"

typedef struct {
    int x, y, width, height;
    char *text;
    Font font;
    Color textColor;
    Color bgColor;
    Color pressedBgColor;
    bool isPressed;
    void (*onClick)(void* arg);
    void* arg;
    bool *buffer;
    int frameId;
} Button;

Button* createButton(int x, int y, int w, int h, char *text, Color textColor, Color bgColor, Color pressedBgColor, Font font, void (*onClick)(void*), void* arg);

#endif
