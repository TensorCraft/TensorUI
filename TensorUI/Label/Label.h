#ifndef LABEL_H
#define LABEL_H
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
    unsigned char *buffer; // 0-255 alpha values
    Font font;
    void (*preRender) (void *self);
    Color (*getPixel)(void *self, int x, int y);
    int alignment;
    int frameId;
    bool isMultiLine;
    int wrapWidth;
    bool dirty;
} Label;

Label* createLabel(int x, int y, int w, int h, const char *text, Color color, Color bgcolor, Font font);
void updateLabel(Label* label, const char* text);

#endif
