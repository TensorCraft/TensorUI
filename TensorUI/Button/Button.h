#ifndef BUTTON_H
#define BUTTON_H

#include "../Label/Label.h"
#include "../Color/color.h"
#include "../Font/font.h"

typedef struct {
    int width, height;
    char *text;
    Font font;
    Color textColor;
    Color bgColor;
    Color pressedBgColor;
    bool isPressed;
    void (*onClick)(void* arg);
    void* arg;
    
    unsigned char *buffer;
    char *lastText;
    int frameId;
    
    float rippleRadius;
    float rippleOpacity;
    int cornerRadius;
} Button;

Button* createButton(int x, int y, int w, int h, char *text, Color textColor, Color bgColor, Color pressedBgColor, Font font, void (*onClick)(void*), void* arg);
void updateButtonText(Button *btn, const char *text);
void setButtonCornerRadius(Button *btn, int radius);
void setButtonColors(Button *btn, Color textColor, Color bgColor, Color pressedBgColor);

#endif
