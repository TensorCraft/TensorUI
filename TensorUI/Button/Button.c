#include "Button.h"
#include <stdlib.h>
#include <string.h>
#include "../../hal/screen/screen.h"

// Helper to check if a pixel is inside a rounded rect
static bool isInsideRoundedRect(int x, int y, int w, int h, int r) {
    if (x < r && y < r) { // Top-left
        return (x - r) * (x - r) + (y - r) * (y - r) <= r * r;
    }
    if (x >= w - r && y < r) { // Top-right
        return (x - (w - r - 1)) * (x - (w - r - 1)) + (y - r) * (y - r) <= r * r;
    }
    if (x < r && y >= h - r) { // Bottom-left
        return (x - r) * (x - r) + (y - (h - r - 1)) * (y - (h - r - 1)) <= r * r;
    }
    if (x >= w - r && y >= h - r) { // Bottom-right
        return (x - (w - r - 1)) * (x - (w - r - 1)) + (y - (h - r - 1)) * (y - (h - r - 1)) <= r * r;
    }
    return true;
}

static Color buttonGetPixel(void *self, int x, int y) {
    Button *btn = (Button *)self;
    int cornerRadius = 8;

    if (!isInsideRoundedRect(x, y, btn->width, btn->height, cornerRadius)) {
        return COLOR_TRANSPARENT;
    }

    // Check text buffer
    if (btn->buffer && btn->buffer[y * btn->width + x]) {
        return btn->textColor;
    }

    return btn->isPressed ? btn->pressedBgColor : btn->bgColor;
}

static void buttonPreRender(void *self) {
    Button *btn = (Button *)self;
    int textWidth = getTextWidth(btn->text, btn->font);
    int textHeight = getTextHeight(btn->font);

    int offsetX = (btn->width - textWidth) / 2;
    int offsetY = (btn->height - textHeight) / 2;

    if (btn->buffer) {
        free(btn->buffer);
    }
    btn->buffer = (bool *)calloc(btn->width * btn->height, sizeof(bool));
    
    bool *textBitmap = getTextBitmap(btn->font, btn->text);
    if (textBitmap) {
        for (int i = 0; i < textHeight; i++) {
            for (int j = 0; j < textWidth; j++) {
                int targetX = j + offsetX;
                int targetY = i + offsetY;
                if (targetX >= 0 && targetX < btn->width && targetY >= 0 && targetY < btn->height) {
                    btn->buffer[targetY * btn->width + targetX] = textBitmap[i * textWidth + j];
                }
            }
        }
    }
}

static void buttonOnClickInternal(void *self) {
    Button *btn = (Button *)self;
    if (btn->onClick) {
        btn->onClick(btn->arg);
    }
}

static void buttonOnTouchInternal(void *self, bool isDown) {
    Button *btn = (Button *)self;
    btn->isPressed = isDown;
    // Signal for render update if needed?
    // In our case, updateScreen redraws everything if renderFlag is true.
    // We should probably set renderFlag = true in screen.c when touch happens.
}

Button* createButton(int x, int y, int w, int h, char *text, Color textColor, Color bgColor, Color pressedBgColor, Font font, void (*onClick)(void*), void* arg) {
    Button *btn = (Button *)malloc(sizeof(Button));
    btn->x = x;
    btn->y = y;
    btn->width = w;
    btn->height = h;
    btn->text = strdup(text);
    btn->textColor = textColor;
    btn->bgColor = bgColor;
    btn->pressedBgColor = pressedBgColor;
    btn->font = font;
    btn->isPressed = false;
    btn->onClick = onClick;
    btn->arg = arg;
    btn->buffer = NULL;

    buttonPreRender(btn);
    btn->frameId = requestFrame(w, h, x, y, btn, buttonPreRender, buttonGetPixel, buttonOnClickInternal, buttonOnTouchInternal);
    return btn;
}
