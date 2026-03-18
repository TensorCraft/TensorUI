#ifndef CANVAS_H
#define CANVAS_H

#include "../Color/color.h"
#include <stdbool.h>

typedef struct {
    int x, y, width, height;
    Color *buffer;
    Color currentColor;
    Color backgroundColor;
    int brushSize;
    int frameId;
    bool isDragging;
    int lastX, lastY;
} Canvas;

Canvas* createCanvas(int x, int y, int w, int h);
void setCanvasColor(Canvas *canvas, Color c);
void setCanvasBrushSize(Canvas *canvas, int size);
void clearCanvas(Canvas *canvas);
void drawCircleOnCanvas(Canvas *cv, int cx, int cy, int r, Color c, bool fill);
void drawRectOnCanvas(Canvas *cv, int x, int y, int w, int h, Color c, bool fill);
Color getCanvasPixel(Canvas *canvas, int x, int y);


#endif
