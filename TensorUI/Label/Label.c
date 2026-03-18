#include <stdbool.h>
#include "../../hal/stdio/stdio.h"
#include "../../hal/mem/mem.h"
#include "../../hal/str/str.h"
#include "Label.h"
#include "../../hal/screen/screen.h"
#include "../Font/font.h"



int max(int a, int b)
{
    return a > b ? a : b;
}

Color getPixel(void *self, int x, int y)
{
    Label *label = (Label *)self;
    unsigned char alpha = label->buffer[y * label->width + x];

    if (alpha == 0) return label->bgcolor;
    if (alpha == 255) return label->color;

    // Linear blending
    Color c;
    c.r = (label->color.r * alpha + label->bgcolor.r * (255 - alpha)) / 255;
    c.g = (label->color.g * alpha + label->bgcolor.g * (255 - alpha)) / 255;
    c.b = (label->color.b * alpha + label->bgcolor.b * (255 - alpha)) / 255;
    c.transparent = false;
    return c;
}

void preRender(void *self)
{
    Label *label = (Label *)self;
    if (!label->dirty) return;
    invalidateFrame(label->frameId);
    label->dirty = false;
    
    int fontHeight = getTextHeight(label->font);

    if (!label->text || label->text[0] == '\0') {
        label->width = 1; label->height = fontHeight;
        label->buffer = (unsigned char *)hal_realloc(label->buffer, 1);
        return;
    }
    
    if (label->isMultiLine && label->wrapWidth > 0) {
        // Multi-line wrapping logic
        int currentX = 0;
        int currentY = 0;
        int maxW = 0;
        
        // 1. First pass: Calculate required height
        char *textCopy = hal_strdup(label->text);
        if (!textCopy) return;
        char *token = hal_strtok(textCopy, " ");
        int lineCount = 1;

        while (token != NULL) {
            int wordW = getTextWidth(token, label->font);
            // Cap a single word's width to avoid horizontal explosion in buffer 
            if (wordW > label->wrapWidth) wordW = label->wrapWidth;
            int spaceW = getTextWidth(" ", label->font);

            if (currentX + wordW > label->wrapWidth) {
                currentX = 0;
                currentY += fontHeight + 2; 
                lineCount++;
            }
            currentX += wordW + spaceW;
            if (currentX > maxW) maxW = currentX;
            token = hal_strtok(NULL, " ");
            
            // Safety cap: don't let a label grow infinitely (limit to 2000px height)
            if (currentY + fontHeight > 2000) break;
        }
        hal_free(textCopy);

        int totalH = lineCount * (fontHeight + 2);
        if (totalH > 2000) totalH = 2000;
        label->width = label->wrapWidth;
        label->height = totalH;

        // 2. Second pass: Draw to buffer
        size_t bufSize = (size_t)label->width * (size_t)label->height;
        label->buffer = (unsigned char *)hal_realloc(label->buffer, bufSize);
        if (!label->buffer) return;
        hal_memset(label->buffer, 0, bufSize);

        textCopy = hal_strdup(label->text);
        if (!textCopy) return;
        token = hal_strtok(textCopy, " ");
        currentX = 0;
        currentY = 0;

        while (token != NULL) {
            int wordW = getTextWidth(token, label->font);
            int origWordW = wordW;
            if (wordW > label->wrapWidth) wordW = label->wrapWidth;
            int spaceW = getTextWidth(" ", label->font);

            if (currentX + wordW > label->wrapWidth) {
                currentX = 0;
                currentY += fontHeight + 2;
            }
            if (currentY + fontHeight > label->height) break;

            bool *bitmap = getTextBitmap(label->font, token);
            if (bitmap) {
                for (int i = 0; i < fontHeight; i++) {
                    for (int j = 0; j < wordW; j++) {
                        int tx = currentX + j;
                        int ty = currentY + i;
                        if (tx < label->width && ty < label->height && bitmap[i * origWordW + j]) {
                            label->buffer[ty * label->width + tx] = 255;
                        }
                    }
                }
                hal_free(bitmap);
            }

            currentX += wordW + spaceW;
            token = hal_strtok(NULL, " ");
        }
        hal_free(textCopy);

    } else {
        // Single line logic
        int textWidth = getTextWidth(label->text, label->font);
        int textHeight = fontHeight;
        int targetWidth = label->width > 0 ? label->width : textWidth;
        int targetHeight = label->height > 0 ? label->height : textHeight;
        label->width = (targetWidth > 1) ? targetWidth : 1;
        label->height = (targetHeight > 1) ? targetHeight : 1;
        
        int offsetX = 0;
        int offsetY = 0;
        if (ALIGNMENT_CENTER_HORIZONTAL & label->alignment)
            offsetX = (label->width - textWidth) / 2;
        if (ALIGNMENT_RIGHT & label->alignment)
            offsetX = label->width - textWidth;
        if (ALIGNMENT_CENTER_VERTICAL & label->alignment)
            offsetY = (label->height - textHeight) / 2;
        if (ALIGNMENT_BOTTOM & label->alignment)
            offsetY = label->height - textHeight;

        bool *bitmap = getTextBitmap(label->font, label->text);
        if (bitmap) {
            size_t bufSize = (size_t)label->width * (size_t)label->height;
            label->buffer = (unsigned char *)hal_realloc(label->buffer, bufSize);
            if (label->buffer) {
                hal_memset(label->buffer, 0, bufSize);
                for (int i = 0; i < textHeight; i++) {
                    for (int j = 0; j < textWidth; j++) {
                        if (bitmap[i * textWidth + j]) {
                            label->buffer[(i + offsetY) * label->width + j + offsetX] = 255;
                        }
                    }
                }
            }
            hal_free(bitmap);
        }
    }

    // sync frame size
    Frames[label->frameId].width = label->width;
    Frames[label->frameId].height = label->height;
    invalidateFrame(label->frameId);

    // Anti-aliasing pass
    if (label->buffer) {
        for (int i = 1; i < label->height - 1; i++) {
            for (int j = 1; j < label->width - 1; j++) {
                int idx = i * label->width + j;
                if (label->buffer[idx] == 0) {
                    int sum = 0;
                    if (label->buffer[idx-1] == 255) sum += 64;
                    if (label->buffer[idx+1] == 255) sum += 64;
                    if (label->buffer[idx-label->width] == 255) sum += 64;
                    if (label->buffer[idx+label->width] == 255) sum += 64;
                    if (sum > 0) label->buffer[idx] = sum > 128 ? 128 : sum;
                }
            }
        }
    }
}


void labelOnDestroy(void *self) {
    Label *label = (Label *)self;
    if (label->text) hal_free(label->text);
    if (label->buffer) hal_free(label->buffer);
    hal_free(label);
}

void LabelOnclick(void *self) {
    hal_printf("Label onclicked\n");
}

Label* createLabel(int x, int y, int w, int h, const char *text, Color color, Color bgcolor, Font font)
{
    Label *label = (Label *)hal_malloc(sizeof(Label));
    label->x = x;
    label->y = y;
    label->width = w;
    label->height = h;
    label->text = hal_strdup(text);
    label->color = color;
    label->font = font;
    label->bgcolor = bgcolor;
    label->getPixel = getPixel;
    label->preRender = preRender;
    label->alignment = ALIGNMENT_LEFT | ALIGNMENT_CENTER_VERTICAL;
    label->isMultiLine = false;
    label->wrapWidth = 0;
    label->dirty = true;
    int textHeight = getTextHeight(label->font);
    label->height = (textHeight > h) ? textHeight : h;
    label->buffer = (unsigned char *)hal_malloc((label->width * label->height));
    if (label->buffer) hal_memset(label->buffer, 0, label->width * label->height);
    label->frameId = requestFrame(label->width, label->height, x, y, label, preRender, getPixel, LabelOnclick, NULL);
    if (label->frameId == -1) {
        hal_free(label->text);
        if (label->buffer) hal_free(label->buffer);
        hal_free(label);
        return NULL;
    }
    Frames[label->frameId].onDestroy = labelOnDestroy;
    return label;
}


void updateLabel(Label* label, const char* text) {
    if (label->text) hal_free(label->text);
    label->text = hal_strdup(text);
    label->dirty = true;
    invalidateFrame(label->frameId);
}
