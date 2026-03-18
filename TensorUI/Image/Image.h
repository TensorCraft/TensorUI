#ifndef UI_IMAGE_H
#define UI_IMAGE_H

#include "../Color/color.h"
#include "../../hal/screen/screen.h"

typedef struct {
    int x, y, width, height;
    const Color *pixels; // Pointer to static bitmap data
    int imgW, imgH;
    int frameId;
    int borderRadius;
} UIImage;

// Create image from existing memory bitmap
UIImage* createUIImageFromBitmap(int x, int y, int w, int h, const Color* data, int srcW, int srcH);
void setUIImageBorderRadius(UIImage* img, int radius);

#endif
