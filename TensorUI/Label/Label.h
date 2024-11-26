#ifndef LABEL_H
#define LABEL_H
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../../font/font.h"
#include "../../hal/screen/screen.h"

extern const int ALIGNMENT_TOP;
extern const int ALIGNMENT_BOTTOM;
extern const int ALIGNMENT_MIDDLE;
extern const int ALIGNMENT_LEFT;
extern const int ALIGNMENT_RIGHT;
extern const int ALIGNMENT_CENTER;

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
} Label;

Label createLabel(int x, int y, int w, int h, char *text, Color color, Color bgcolor, Font font);

#endif