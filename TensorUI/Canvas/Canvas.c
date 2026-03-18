#include "Canvas.h"
#include "../../hal/mem/mem.h"
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

static void drawBrushAt(Canvas *cv, int x, int y) {
    int r = cv->brushSize;
    int r2 = r * r;
    for (int i = -r; i <= r; i++) {
        for (int j = -r; j <= r; j++) {
            if (i*i + j*j <= r2) {
                int px = x + i;
                int py = y + j;
                if (px >= 0 && px < cv->width && py >= 0 && py < cv->height) {
                    cv->buffer[py * cv->width + px] = cv->currentColor;
                }
            }
        }
    }
}

static void drawLineOnCanvas(Canvas *cv, int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        drawBrushAt(cv, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    invalidateFrame(cv->frameId);
}

void drawCircleOnCanvas(Canvas *cv, int cx, int cy, int r, Color c, bool fill) {
    int r2 = r * r;
    int rInner2 = (r-2)*(r-2);
    for (int i = -r; i <= r; i++) {
        for (int j = -r; j <= r; j++) {
            int d2 = i*i + j*j;
            if (d2 <= r2) {
                if (fill || d2 > rInner2) {
                    int px = cx + i;
                    int py = cy + j;
                    if (px >= 0 && px < cv->width && py >= 0 && py < cv->height) {
                        cv->buffer[py * cv->width + px] = c;
                    }
                }
            }
        }
    }
    invalidateFrame(cv->frameId);
}

void drawRectOnCanvas(Canvas *cv, int x, int y, int w, int h, Color c, bool fill) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            bool isEdge = (i == 0 || i == w-1 || j == 0 || j == h-1);
            if (fill || isEdge) {
                int px = x + i;
                int py = y + j;
                if (px >= 0 && px < cv->width && py >= 0 && py < cv->height) {
                    cv->buffer[py * cv->width + px] = c;
                }
            }
        }
    }
    invalidateFrame(cv->frameId);
}

static void canvasOnDestroy(void *self) {
    Canvas *cv = (Canvas *)self;
    if (cv->buffer) hal_free(cv->buffer);
    hal_free(cv);
}

static void canvasOnTouch(void *self, bool isDown) {
    Canvas *cv = (Canvas *)self;
    cv->isDragging = isDown;
    Frames[cv->frameId].continuousRender = isDown;
    if (isDown) {
        int mx, my;
        getTouchState(&mx, &my);
        cv->lastX = mx - Frames[cv->frameId].x;
        cv->lastY = my - Frames[cv->frameId].y;
        drawBrushAt(cv, cv->lastX, cv->lastY);
    }
    invalidateFrame(cv->frameId);
}

static void canvasCancelInteraction(void *self) {
    Canvas *cv = (Canvas *)self;
    cv->isDragging = false;
    Frames[cv->frameId].continuousRender = false;
    invalidateFrame(cv->frameId);
}

static void canvasPreRender(void *self) {
    Canvas *cv = (Canvas *)self;
    cv->x = Frames[cv->frameId].x;
    cv->y = Frames[cv->frameId].y;

    if (cv->isDragging) {
        int mx, my;
        getTouchState(&mx, &my);
        int localX = mx - cv->x;
        int localY = my - cv->y;
        
        if (localX != cv->lastX || localY != cv->lastY) {
            drawLineOnCanvas(cv, cv->lastX, cv->lastY, localX, localY);
            cv->lastX = localX;
            cv->lastY = localY;
        }
    }
}

Canvas* createCanvas(int x, int y, int w, int h) {
    Canvas *cv = (Canvas *)hal_malloc(sizeof(Canvas));
    cv->x = x;
    cv->y = y;
    cv->width = w;
    cv->height = h;
    cv->buffer = (Color *)hal_malloc(w * h * sizeof(Color));
    cv->backgroundColor = (Color){25, 25, 25, false};
    cv->currentColor = COLOR_WHITE;
    cv->brushSize = 3;
    cv->isDragging = false;
    
    for(int i=0; i<w*h; i++) {
        cv->buffer[i] = cv->backgroundColor;
    }
    
    cv->frameId = requestFrame(w, h, x, y, cv, canvasPreRender, canvasGetPixel, NULL, canvasOnTouch);
    Frames[cv->frameId].scrollType = 3;
    Frames[cv->frameId].onCancelInteraction = canvasCancelInteraction;
    Frames[cv->frameId].onDestroy = canvasOnDestroy;
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
        canvas->buffer[i] = canvas->backgroundColor;
    }
    invalidateFrame(canvas->frameId);
}

Color getCanvasPixel(Canvas *canvas, int x, int y) {
    if (x >= 0 && x < canvas->width && y >= 0 && y < canvas->height) {
        return canvas->buffer[y * canvas->width + x];
    }
    return COLOR_TRANSPARENT;
}
