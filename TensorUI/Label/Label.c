#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "Label.h"
#include "../../hal/screen/screen.h"
#include "../../font/font.h"

const int ALIGNMENT_TOP = 0;
const int ALIGNMENT_BOTTOM = 1;
const int ALIGNMENT_MIDDLE = 1 << 1;
const int ALIGNMENT_LEFT = 0;
const int ALIGNMENT_RIGHT = 1 << 2;
const int ALIGNMENT_CENTER = 1 << 3;

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
    if (ALIGNMENT_CENTER & label->alignment)
        offsetX = (label->width - textWidth) / 2;
    if (ALIGNMENT_RIGHT & label->alignment)
        offsetX = label->width - textWidth;
    if (ALIGNMENT_MIDDLE & label->alignment)
        offsetY = (label->height - textHeight) / 2;
    if (ALIGNMENT_BOTTOM & label->alignment)
        offsetY = label->height - textHeight;
    bool *bitmap = getTextBitmap(label->font, label->text);
    label->buffer = (char *)realloc(label->buffer, (label->width * label->height) * sizeof(char));
    memset(label->buffer, 0, (label->width * label->height) * sizeof(char));
    memset(label->buffer, 0, sizeof(bool) * label->width * label->height);
    for (int i = 0; i < textHeight; i++)
        for (int j = 0; j < textWidth; j++)
            label->buffer[(i + offsetY) * label->width + j + offsetX] = bitmap[i * textWidth + j];
}

Label createLabel(int x, int y, int w, int h, char *text, Color color, Color bgcolor, Font font)
{
    Label label;
    label.x = x;
    label.y = y;
    label.width = w;
    label.height = h;
    label.text = text;
    label.color = color;
    label.font = font;
    label.bgcolor = bgcolor;
    label.getPixel = getPixel;
    label.preRender = preRender;
    label.alignment = ALIGNMENT_LEFT | ALIGNMENT_MIDDLE;
    int textWidth = getTextWidth(label.text, label.font);
    int textHeight = getTextHeight(label.font);
    label.width = max(textWidth, label.width);
    label.height = max(textHeight, label.height);
    label.buffer = (char *)malloc((label.width * label.height) * sizeof(char));
    requestFrame(label.width, label.height, x, y, &label, preRender, getPixel, NULL, NULL);
    return label;
}