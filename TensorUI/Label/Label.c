#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "Label.h"
#include "../../hal/screen/screen.h"
#include "../Font/font.h"



int max(int a, int b)
{
    return a > b ? a : b;
}

Color getPixel(void *self, int x, int y)
{
    Label *label = (Label *)self;

    if (label->buffer[y * label->width + x])
        return label->color;

    return label->bgcolor;
}

void preRender(void *self)
{

    Label *label = (Label *)self;
    int textWidth = getTextWidth(label->text, label->font);
    int textHeight = getTextHeight(label->font);
    label->width = max(textWidth, label->width);
    label->height = max(textHeight, label->height);
    int offsetX = 0;
    int offsetY = 0;
    if (ALIGNMENT_CENTER_HORIZONTAL & label->alignment)
        offsetX = (label->width - textWidth) / 2;
    if (ALIGNMENT_RIGHT & label->alignment)
        offsetX = label->width - textWidth;
    if (ALIGNMENT_CENTER_VERTICAL & label->alignment)
        offsetY = (label->height - textHeight) / 2;
    if (ALIGNMENT_BOTTOM & label->alignment)
        offsetY = label->height - textHeight;
    bool *bitmap = getTextBitmap(label->font, label->text);
    label->buffer = (bool *)realloc(label->buffer, (label->width * label->height) * sizeof(bool));
    memset(label->buffer, 0, sizeof(bool) * label->width * label->height);
    for (int i = 0; i < textHeight; i++)
        for (int j = 0; j < textWidth; j++)
            label->buffer[(i + offsetY) * label->width + j + offsetX] = bitmap[i * textWidth + j];
}

void LabelOnclick(void *self) {
    printf("Label onclicked\n");
}

Label* createLabel(int x, int y, int w, int h, char *text, Color color, Color bgcolor, Font font)
{
    Label *label = (Label *)malloc(sizeof(Label));
    label->x = x;
    label->y = y;
    label->width = w;
    label->height = h;
    label->text = strdup(text);
    label->color = color;
    label->font = font;
    label->bgcolor = bgcolor;
    label->getPixel = getPixel;
    label->preRender = preRender;
    label->alignment = ALIGNMENT_LEFT | ALIGNMENT_CENTER_VERTICAL;
    int textWidth = getTextWidth(label->text, label->font);
    int textHeight = getTextHeight(label->font);
    label->width = max(textWidth, label->width);
    label->height = max(textHeight, label->height);
    label->buffer = (bool *)malloc((label->width * label->height) * sizeof(bool));
    label->frameId = requestFrame(label->width, label->height, x, y, label, preRender, getPixel, LabelOnclick, NULL);
    return label;
}

void updateLabel(Label* label, const char* text) {
    if (label->text) free(label->text);
    label->text = strdup(text);
    // Mark for re-rendering if we had a dirty flag, 
    // but preRender is called every frame anyway in screen.c now.
}