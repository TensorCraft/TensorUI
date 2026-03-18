#include "Button.h"
#include "../../hal/math/math.h"
#include "../../hal/mem/mem.h"
#include "../../hal/str/str.h"
#include "../../hal/screen/screen.h"

static bool buttonPointerInside(Button *btn) {
    int mx = currentInputSession.currentX;
    int my = currentInputSession.currentY;
    if (!currentInputSession.active) {
        getTouchState(&mx, &my);
    }

    Frame *frame = &Frames[btn->frameId];
    return mx >= frame->x && mx < frame->x + frame->width &&
           my >= frame->y && my < frame->y + frame->height;
}

static Color buttonGetPixel(void *self, int x, int y) {
    Button *btn = (Button *)self;
    int w = btn->width;
    int h = btn->height;
    int r = (btn->cornerRadius > 0) ? btn->cornerRadius : (h / 2);
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    // Rounded Rectangle Logic
    bool inCorner = false;
    int cx = -1, cy = -1;

    if (x < r && y < r) { cx = r; cy = r; inCorner = true; } // Top Left
    else if (x < r && y >= h - r) { cx = r; cy = h - r - 1; inCorner = true; } // Bottom Left
    else if (x >= w - r && y < r) { cx = w - r - 1; cy = r; inCorner = true; } // Top Right
    else if (x >= w - r && y >= h - r) { cx = w - r - 1; cy = h - r - 1; inCorner = true; } // Bottom Right

    if (inCorner) {
        int dx = x - cx;
        int dy = y - cy;
        if (dx*dx + dy*dy > r * r) return COLOR_TRANSPARENT;
    }

    Color baseColor = btn->isPressed ? btn->pressedBgColor : btn->bgColor;
    if (baseColor.transparent) return COLOR_TRANSPARENT;

    // Text Overlay
    if (btn->buffer) {
        unsigned char alpha = btn->buffer[y * w + x];
        if (alpha == 255) return btn->textColor;
        if (alpha > 0) {
            Color c;
            c.r = (btn->textColor.r * alpha + baseColor.r * (255 - alpha)) / 255;
            c.g = (btn->textColor.g * alpha + baseColor.g * (255 - alpha)) / 255;
            c.b = (btn->textColor.b * alpha + baseColor.b * (255 - alpha)) / 255;
            c.transparent = false;
            return c;
        }
    }

    // Ripple
    if (btn->rippleOpacity > 0.05f) {
        int dx = x - w/2;
        int dy = y - h/2;
        if (dx*dx + dy*dy < btn->rippleRadius * btn->rippleRadius) {
            float a = btn->rippleOpacity * 0.3f;
            baseColor.r = (unsigned char)(255 * a + baseColor.r * (1.0f - a));
            baseColor.g = (unsigned char)(255 * a + baseColor.g * (1.0f - a));
            baseColor.b = (unsigned char)(255 * a + baseColor.b * (1.0f - a));
        }
    }

    return baseColor;
}

static void buttonPreRender(void *self) {
    Button *btn = (Button *)self;
    
    // Cache check
    if (btn->buffer && btn->lastText && btn->text && hal_strcmp(btn->text, btn->lastText) == 0) {
        return;
    }

    if (!btn->buffer) btn->buffer = (unsigned char *)hal_malloc(btn->width * btn->height);
    hal_memset(btn->buffer, 0, btn->width * btn->height);
    
    if (btn->lastText) hal_free(btn->lastText);
    btn->lastText = hal_strdup(btn->text ? btn->text : "");

    if (!btn->text || btn->text[0] == '\0') return;

    int tw = getTextWidth(btn->text, btn->font);
    int th = getTextHeight(btn->font);
    int ox = (btn->width - tw) / 2;
    int oy = (btn->height - th) / 2;

    bool *bitmap = getTextBitmap(btn->font, btn->text);
    if (bitmap) {
        for (int i = 0; i < th; i++) {
            for (int j = 0; j < tw; j++) {
                int ty = i + oy;
                int tx = j + ox;
                if (tx >= 0 && tx < btn->width && ty >= 0 && ty < btn->height) {
                    if (bitmap[i * tw + j]) btn->buffer[ty * btn->width + tx] = 255;
                }
            }
        }
        hal_free(bitmap);
    }
}

static void buttonOnDestroy(void *self) {
    Button *btn = (Button *)self;
    if (btn->text) hal_free(btn->text);
    if (btn->lastText) hal_free(btn->lastText);
    if (btn->buffer) hal_free(btn->buffer);
    hal_free(btn);
}

static void buttonOnClickInternal(void *self) {
    Button *btn = (Button *)self;
    if (btn->onClick) btn->onClick(btn->arg);
}

static void buttonOnTouchInternal(void *self, bool isDown) {
    Button *btn = (Button *)self;
    btn->isPressed = isDown;
    Frames[btn->frameId].continuousRender = isDown || btn->rippleOpacity > 0.0f;
    if (isDown) {
        btn->rippleRadius = 0;
        btn->rippleOpacity = 1.0f;
    }
    invalidateFrame(btn->frameId);
}

static void buttonOnUpdate(void *self) {
    Button *btn = (Button *)self;
    bool visualChanged = false;

    if (btn->isPressed) {
        bool shouldRemainPressed = currentInputSession.active && buttonPointerInside(btn);
        if (!shouldRemainPressed) {
            btn->isPressed = false;
            visualChanged = true;
        }
    }

    if (btn->rippleOpacity > 0) {
        btn->rippleRadius += 4.0f;
        btn->rippleOpacity -= 0.1f;
        if (btn->rippleOpacity < 0) btn->rippleOpacity = 0;
        visualChanged = true;
    }

    Frames[btn->frameId].continuousRender = btn->isPressed || btn->rippleOpacity > 0.0f;
    if (visualChanged) invalidateFrame(btn->frameId);
}

Button* createButton(int x, int y, int w, int h, char *text, Color textColor, Color bgColor, Color pressedBgColor, Font font, void (*onClick)(void*), void* arg) {
    Button *btn = (Button *)hal_malloc(sizeof(Button));
    btn->width = w; btn->height = h;
    btn->text = hal_strdup(text ? text : "");
    btn->textColor = textColor;
    btn->bgColor = bgColor;
    btn->pressedBgColor = pressedBgColor;
    btn->font = font;
    btn->isPressed = false;
    btn->onClick = onClick;
    btn->arg = arg;
    btn->buffer = NULL;
    btn->lastText = NULL;
    btn->rippleRadius = 0;
    btn->rippleOpacity = 0;
    btn->cornerRadius = -1;

    buttonPreRender(btn);
    btn->frameId = requestFrame(w, h, x, y, btn, buttonPreRender, buttonGetPixel, buttonOnClickInternal, buttonOnTouchInternal);
    if (btn->frameId == -1) {
        if (btn->text) hal_free(btn->text);
        if (btn->lastText) hal_free(btn->lastText);
        if (btn->buffer) hal_free(btn->buffer);
        hal_free(btn);
        return NULL;
    }
    Frames[btn->frameId].onUpdate = buttonOnUpdate;
    Frames[btn->frameId].onDestroy = buttonOnDestroy;
    return btn;
}

void updateButtonText(Button *btn, const char *text) {
    if (!btn) return;
    if (btn->text) hal_free(btn->text);
    btn->text = hal_strdup(text ? text : "");
    invalidateFrame(btn->frameId);
}

void setButtonCornerRadius(Button *btn, int radius) {
    if (!btn) return;
    btn->cornerRadius = radius;
    invalidateFrame(btn->frameId);
}

void setButtonColors(Button *btn, Color textColor, Color bgColor, Color pressedBgColor) {
    if (!btn) return;
    btn->textColor = textColor;
    btn->bgColor = bgColor;
    btn->pressedBgColor = pressedBgColor;
    invalidateFrame(btn->frameId);
}
