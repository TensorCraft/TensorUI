#include "ProgressBar.h"
#include <stdlib.h>
#include <stdbool.h>
#include "../../hal/screen/screen.h"

static Color progressGetPixel(void *self, int x, int y) {
    ProgressBar *pb = (ProgressBar *)self;
    int fillWidth = (int)(pb->width * pb->progress);
    
    if (x < fillWidth) {
        return pb->barColor;
    }
    return pb->bgColor;
}

ProgressBar* createProgressBar(int x, int y, int w, int h, float initialProgress) {
    ProgressBar *pb = (ProgressBar *)malloc(sizeof(ProgressBar));
    pb->x = x;
    pb->y = y;
    pb->width = w;
    pb->height = h;
    pb->progress = initialProgress;
    pb->barColor = (Color){0, 122, 255, false}; // Blue
    pb->bgColor = (Color){50, 50, 50, false};   // Gray

    requestFrame(w, h, x, y, pb, NULL, progressGetPixel, NULL, NULL);
    return pb;
}

extern bool renderFlag;

void setProgress(ProgressBar *pb, float progress) {
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;
    if (pb->progress != progress) {
        pb->progress = progress;
        renderFlag = true;
    }
}
