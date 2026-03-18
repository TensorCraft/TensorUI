#include "Image.h"
#include "../../hal/mem/mem.h"

extern Frame Frames[];
extern bool renderFlag;

static Color imageGetPixel(void *self, int x, int y) {
    UIImage *img = (UIImage *)self;
    if (!img->pixels) return COLOR_TRANSPARENT;

    int w = img->width;
    int h = img->height;
    int r = img->borderRadius;

    // Corner Check for Border Radius
    if (r > 0) {
        int cx = -1, cy = -1;
        if (x < r && y < r) { cx = r; cy = r; }
        else if (x >= w - r && y < r) { cx = w - r - 1; cy = r; }
        else if (x < r && y >= h - r) { cx = r; cy = h - r - 1; }
        else if (x >= w - r && y >= h - r) { cx = w - r - 1; cy = h - r - 1; }

        if (cx != -1) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx * dx + dy * dy > r * r) return COLOR_TRANSPARENT;
        }
    }

    // Nearest Neighbor scaling for bitmap
    int srcX = (x * img->imgW) / w;
    int srcY = (y * img->imgH) / h;
    
    if (srcX >= 0 && srcX < img->imgW && srcY >= 0 && srcY < img->imgH) {
        return img->pixels[srcY * img->imgW + srcX];
    }

    return COLOR_TRANSPARENT;
}

UIImage* createUIImageFromBitmap(int x, int y, int w, int h, const Color* data, int srcW, int srcH) {
    UIImage *uiImg = (UIImage *)hal_malloc(sizeof(UIImage));
    uiImg->x = x;
    uiImg->y = y;
    uiImg->width = w;
    uiImg->height = h;
    uiImg->borderRadius = 0;
    uiImg->pixels = data;
    uiImg->imgW = srcW;
    uiImg->imgH = srcH;

    uiImg->frameId = requestFrame(w, h, x, y, uiImg, NULL, imageGetPixel, NULL, NULL);
    return uiImg;
}

void setUIImageBorderRadius(UIImage* img, int radius) {
    img->borderRadius = radius;
    renderFlag = true;
}
