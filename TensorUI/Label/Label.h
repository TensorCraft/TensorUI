#ifndef LABEL_H
#define LABEL_H
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../Font/font.h"
#include "../../hal/screen/screen.h"



typedef struct {
    int x;
    int y;
    int width;
    int height;
    char *text;
    Color color;
    Color bgcolor;
    Font font;
    void (*preRender) (void *self);
    Color (*getPixel)(void *self, int x, int y);
    int alignment;
    bool* buffer;
    int frameId;
} Label;

Label* createLabel(int x, int y, int w, int h, char *text, Color color, Color bgcolor, Font font);
void updateLabel(Label* label, const char* text);

#endif