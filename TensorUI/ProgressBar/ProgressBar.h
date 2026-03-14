#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "../Color/color.h"

typedef struct {
    int x, y, width, height;
    float progress; // 0.0 to 1.0
    Color barColor;
    Color bgColor;
} ProgressBar;

ProgressBar* createProgressBar(int x, int y, int w, int h, float initialProgress);
void setProgress(ProgressBar *pb, float progress);

#endif
