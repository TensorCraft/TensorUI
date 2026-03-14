#include "Canvas.h"
#include <stdlib.h>
#include <math.h>
#include "../../hal/screen/screen.h"

extern Frame Frames[];
extern bool renderFlag;

static Color canvasGetPixel(void *self, int x, int y) {
    Canvas *cv = (Canvas *)self;
    if (x >= 0 && x < cv->width && y >= 0 && y < cv->height) {
        return cv->buffer[y * cv->width + x];
    }
    return COLOR_TRANSPARENT;
}

static void drawLineOnCanvas(Canvas *cv, int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        // draw brush circle at x0, y0
        int r = cv->brushSize;
        for (int i = -r; i <= r; i++) {
            for (int j = -r; j <= r; j++) {
                if (i*i + j*j <= r*r) {
                    int px = x0 + i;
                    int py = y0 + j;
                    if (px >= 0 && px < cv->width && py >= 0 && py < cv->height) {
                        cv->buffer[py * cv->width + px] = cv->currentColor;
                    }
                }
            }
        }
        
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    renderFlag = true;
}

static void canvasOnTouch(void *self, bool isDown) {
    Canvas *cv = (Canvas *)self;
    cv->isDragging = isDown;
    if (isDown) {
        int mx, my;
        getTouchState(&mx, &my);
        cv->lastX = mx - Frames[cv->frameId].x;
        cv->lastY = my - Frames[cv->frameId].y;
        drawLineOnCanvas(cv, cv->lastX, cv->lastY, cv->lastX, cv->lastY);
    }
}

static void canvasPreRender(void *self) {
    Canvas *cv = (Canvas *)self;
    
    // Sync position (if inside a stack)
    cv->x = Frames[cv->frameId].x;
    cv->y = Frames[cv->frameId].y;

    if (cv->isDragging) {
        int mx, my;
        getTouchState(&mx, &my);
        int localX = mx - cv->x;
        int localY = my - cv->y;
        
        // draw line from lastX/Y to localX/Y
        if (localX != cv->lastX || localY != cv->lastY) {
            drawLineOnCanvas(cv, cv->lastX, cv->lastY, localX, localY);
            cv->lastX = localX;
            cv->lastY = localY;
        }
    }
}

Canvas* createCanvas(int x, int y, int w, int h) {
    Canvas *cv = (Canvas *)malloc(sizeof(Canvas));
    cv->x = x;
    cv->y = y;
    cv->width = w;
    cv->height = h;
    cv->buffer = (Color *)malloc(w * h * sizeof(Color));
    cv->currentColor = COLOR_WHITE;
    cv->brushSize = 3;
    cv->isDragging = false;
    
    for(int i=0; i<w*h; i++) {
        cv->buffer[i] = (Color){20, 20, 20, false};
    }
    
    cv->frameId = requestFrame(w, h, x, y, cv, canvasPreRender, canvasGetPixel, NULL, canvasOnTouch);
    Frames[cv->frameId].scrollType = 3; // Custom scroll type to capture drag focus
    return cv;
}

void setCanvasColor(Canvas *canvas, Color c) {
    canvas->currentColor = c;
}

void setCanvasBrushSize(Canvas *canvas, int size) {
    canvas->brushSize = size;
}

void clearCanvas(Canvas *canvas) {
    for(int i=0; i<canvas->width * canvas->height; i++) {
        canvas->buffer[i] = (Color){20, 20, 20, false};
    }
    renderFlag = true;
}

Color getCanvasPixel(Canvas *canvas, int x, int y) {
    if (x >= 0 && x < canvas->width && y >= 0 && y < canvas->height) {
        return canvas->buffer[y * canvas->width + x];
    }
    return COLOR_TRANSPARENT;
}
