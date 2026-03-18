#include "ProgressBar.h"
#include "../../hal/mem/mem.h"
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
    ProgressBar *pb = (ProgressBar *)hal_malloc(sizeof(ProgressBar));
    pb->x = x;
    pb->y = y;
    pb->width = w;
    pb->height = h;
    pb->progress = initialProgress;
    pb->barColor = (Color){0, 122, 255, false}; // Blue
    pb->bgColor = (Color){50, 50, 50, false};   // Gray

    pb->frameId = requestFrame(w, h, x, y, pb, NULL, progressGetPixel, NULL, NULL);
    if (pb->frameId == -1) {
        hal_free(pb);
        return NULL;
    }
    return pb;
}

void setProgress(ProgressBar *pb, float progress) {
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;
    if (pb->progress != progress) {
        float oldProgress = pb->progress;
        pb->progress = progress;
        int oldFillWidth = (int)(pb->width * oldProgress);
        int newFillWidth = (int)(pb->width * pb->progress);
        int dirtyX = oldFillWidth < newFillWidth ? oldFillWidth : newFillWidth;
        int dirtyW = oldFillWidth > newFillWidth ? oldFillWidth - newFillWidth : newFillWidth - oldFillWidth;
        if (dirtyW <= 0) dirtyW = pb->width;
        invalidateFrameRect(pb->frameId, dirtyX, 0, dirtyW, pb->height);
    }
}
