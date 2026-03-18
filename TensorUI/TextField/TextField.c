#include "TextField.h"
#include "../../hal/mem/mem.h"
#include "../../hal/str/str.h"
#include "../../hal/math/math.h"
#include "../../hal/screen/screen.h"
#include "../../hal/time/time.h"

#define TEXTFIELD_MAX_TRACKED 512
#define TEXTFIELD_PADDING_X 10
#define TEXTFIELD_PADDING_Y 10
#define TEXTFIELD_LONG_PRESS_MS 350
#define TEXTFIELD_TITLE_GAP 6
static char textFieldClipboard[TEXTFIELD_MAX_TRACKED] = {0};

struct TextField {
    int x;
    int y;
    int w;
    int h;
    char *text;
    Font font;
    Color textColor;
    Color bgColor;
    unsigned char *renderMask;
    int textW;
    int textH;
    bool dirty;
    bool ownsText;
    int maxLen;
    bool editingEnabled;
    bool multiline;
    char *placeholder;
    bool *placeholderBitmap;
    int placeholderW;
    int placeholderH;
    int cursorIndex;
    int selectionStart;
    int selectionEnd;
    float scrollX;
    float scrollY;
    int trackedLen;
    int totalTextHeight;
    int contentX;
    int contentY;
    int contentW;
    int contentH;
    int caretX[TEXTFIELD_MAX_TRACKED + 1];
    int caretY[TEXTFIELD_MAX_TRACKED + 1];
    bool isTouchDown;
    bool longPressActive;
    long long touchDownAt;
    int touchStartX;
    int touchStartY;
    int frameId;
    void (*onClick)(void*);
    void *arg;
};

static void textFieldSessionInsertText(void *context, const char *text) {
    textFieldInsertText((TextField *)context, text);
}

static void textFieldSessionBackspace(void *context) {
    textFieldBackspace((TextField *)context);
}

static void textFieldSessionSelectAll(void *context) {
    textFieldSelectAll((TextField *)context);
}

static void textFieldSessionCopy(void *context) {
    textFieldCopy((TextField *)context);
}

static void textFieldSessionCut(void *context) {
    textFieldCut((TextField *)context);
}

static void textFieldSessionPaste(void *context) {
    textFieldPaste((TextField *)context);
}

static void textFieldActivateSession(TextField *tf) {
    if (!tf || !tf->editingEnabled) return;
    registerTextInputTarget((TextInputTarget){
        .frameId = tf->frameId,
        .context = tf,
        .insertText = textFieldSessionInsertText,
        .backspace = textFieldSessionBackspace,
        .selectAll = textFieldSessionSelectAll,
        .copy = textFieldSessionCopy,
        .cut = textFieldSessionCut,
        .paste = textFieldSessionPaste,
    });
}

static void textFieldOnFocus(void *self) {
    TextField *tf = (TextField *)self;
    if (!tf->editingEnabled) return;
    tf->dirty = true;
    renderFlag = true;
}

static void textFieldOnBlur(void *self) {
    TextField *tf = (TextField *)self;
    tf->isTouchDown = false;
    tf->longPressActive = false;
    tf->dirty = true;
    renderFlag = true;
}

static int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static int textFieldLength(TextField *tf) {
    if (!tf || !tf->text) return 0;
    return (int)hal_strlen(tf->text);
}

static bool textFieldHasSelection(TextField *tf) {
    return tf && tf->selectionStart != tf->selectionEnd;
}

static int textFieldSelectionMin(TextField *tf) {
    return (tf->selectionStart < tf->selectionEnd) ? tf->selectionStart : tf->selectionEnd;
}

static int textFieldSelectionMax(TextField *tf) {
    return (tf->selectionStart > tf->selectionEnd) ? tf->selectionStart : tf->selectionEnd;
}

static void textFieldClampIndices(TextField *tf) {
    int len = textFieldLength(tf);
    tf->cursorIndex = clampInt(tf->cursorIndex, 0, len);
    tf->selectionStart = clampInt(tf->selectionStart, 0, len);
    tf->selectionEnd = clampInt(tf->selectionEnd, 0, len);
}

static void textFieldClearSelection(TextField *tf) {
    tf->selectionStart = tf->cursorIndex;
    tf->selectionEnd = tf->cursorIndex;
}

static void textFieldEnsureCapacity(TextField *tf, int desiredLen) {
    if (!tf || !tf->ownsText) return;
    if (desiredLen + 1 <= tf->maxLen) return;

    int newMax = tf->maxLen;
    while (newMax < desiredLen + 1) newMax *= 2;
    char *newText = (char *)hal_realloc(tf->text, (size_t)newMax);
    if (!newText) return;
    tf->text = newText;
    tf->maxLen = newMax;
}

static void textFieldDeleteRange(TextField *tf, int start, int end) {
    int len = textFieldLength(tf);
    start = clampInt(start, 0, len);
    end = clampInt(end, 0, len);
    if (end <= start) return;

    hal_memcpy(tf->text + start, tf->text + end, (size_t)(len - end + 1));
    tf->cursorIndex = start;
    textFieldClearSelection(tf);
    tf->dirty = true;
    renderFlag = true;
}

static void textFieldDeleteSelection(TextField *tf) {
    if (!textFieldHasSelection(tf)) return;
    textFieldDeleteRange(tf, textFieldSelectionMin(tf), textFieldSelectionMax(tf));
}

static void textFieldEnsureCursorVisible(TextField *tf) {
    int lineH = tf->textH > 0 ? tf->textH : 1;
    int len = textFieldLength(tf);
    int caretIndex = clampInt(tf->cursorIndex, 0, len);
    int caretX = tf->caretX[caretIndex];
    int caretY = tf->caretY[caretIndex];

    if (tf->multiline) {
        if (caretY < (int)tf->scrollY) tf->scrollY = (float)caretY;
        if (caretY + lineH > (int)tf->scrollY + tf->contentH) {
            tf->scrollY = (float)(caretY + lineH - tf->contentH);
        }
        int maxScrollY = tf->totalTextHeight - tf->contentH;
        if (maxScrollY < 0) maxScrollY = 0;
        if (tf->scrollY < 0) tf->scrollY = 0;
        if (tf->scrollY > maxScrollY) tf->scrollY = (float)maxScrollY;
        tf->scrollX = 0;
    } else {
        if (caretX < (int)tf->scrollX) tf->scrollX = (float)caretX;
        if (caretX + 2 > (int)tf->scrollX + tf->contentW) {
            tf->scrollX = (float)(caretX + 2 - tf->contentW);
        }
        int maxScrollX = tf->textW - tf->contentW;
        if (maxScrollX < 0) maxScrollX = 0;
        if (tf->scrollX < 0) tf->scrollX = 0;
        if (tf->scrollX > maxScrollX) tf->scrollX = (float)maxScrollX;
        tf->scrollY = 0;
    }
}

static int textFieldHeaderHeight(TextField *tf) {
    if (!tf->placeholder || !tf->placeholder[0]) return 0;
    int h = tf->placeholderH > 0 ? tf->placeholderH : getTextHeight(tf->font);
    if (h < 1) h = 1;
    return h + TEXTFIELD_TITLE_GAP * 2;
}

static int textFieldPlaceholderOffset(TextField *tf) {
    if (!tf->placeholder || !tf->placeholder[0]) return 0;
    int available = tf->w - TEXTFIELD_PADDING_X * 2;
    int overflow = tf->placeholderW - available;
    if (overflow <= 0) return 0;

    long long tick = current_timestamp_ms() / 40;
    int cycle = overflow + available / 2 + 12;
    if (cycle < 1) cycle = 1;
    int phase = (int)(tick % (cycle * 2));
    if (phase >= cycle) phase = cycle * 2 - phase;
    if (phase > overflow) phase = overflow;
    return phase;
}

static void textFieldRebuildLayout(TextField *tf) {
    int len = textFieldLength(tf);
    if (len > TEXTFIELD_MAX_TRACKED - 1) len = TEXTFIELD_MAX_TRACKED - 1;

    tf->trackedLen = len;
    tf->textH = getTextHeight(tf->font);
    if (tf->textH < 1) tf->textH = 1;
    tf->contentX = TEXTFIELD_PADDING_X;
    tf->contentW = tf->w - TEXTFIELD_PADDING_X * 2;
    if (tf->contentW < 1) tf->contentW = 1;

    int headerH = textFieldHeaderHeight(tf);
    if (tf->multiline) {
        tf->contentY = TEXTFIELD_PADDING_Y + headerH;
        tf->contentH = tf->h - tf->contentY - TEXTFIELD_PADDING_Y;
    } else {
        int availableH = tf->h - headerH;
        tf->contentY = headerH + (availableH - tf->textH) / 2;
        tf->contentH = tf->textH;
    }
    if (tf->contentH < 1) tf->contentH = 1;

    int x = 0;
    int y = 0;
    int maxLineW = 0;
    for (int i = 0; i < len; i++) {
        char c = tf->text[i];
        int charW = getCharacterWidth(tf->font, c);
        if (charW < 1) charW = 1;

        if (tf->multiline && c != '\n' && x > 0 && x + charW > tf->contentW) {
            x = 0;
            y += tf->textH + 2;
        }

        tf->caretX[i] = x;
        tf->caretY[i] = y;

        if (c == '\n') {
            if (x > maxLineW) maxLineW = x;
            x = 0;
            y += tf->textH + 2;
            continue;
        }

        x += charW;
        if (x > maxLineW) maxLineW = x;
    }
    tf->caretX[len] = x;
    tf->caretY[len] = y;
    tf->textW = maxLineW;
    tf->totalTextHeight = y + tf->textH;
    if (tf->totalTextHeight < tf->textH) tf->totalTextHeight = tf->textH;

    textFieldClampIndices(tf);
    textFieldEnsureCursorVisible(tf);
}

static int textFieldIndexFromPoint(TextField *tf, int px, int py) {
    int localX = px - Frames[tf->frameId].x - tf->contentX + (int)tf->scrollX;
    int localY = py - Frames[tf->frameId].y - tf->contentY + (int)tf->scrollY;
    if (localX < 0) localX = 0;
    if (localY < 0) localY = 0;

    int len = tf->trackedLen;
    int bestIndex = 0;
    long bestDistance = 1L << 30;
    for (int i = 0; i <= len; i++) {
        int dx = tf->caretX[i] - localX;
        int dy = tf->caretY[i] - localY;
        long dist = (long)dx * dx + (long)dy * dy;
        if (dist < bestDistance) {
            bestDistance = dist;
            bestIndex = i;
        }
    }
    return bestIndex;
}

static void textFieldMoveCursorToPoint(TextField *tf, int px, int py, bool keepSelection) {
    tf->cursorIndex = textFieldIndexFromPoint(tf, px, py);
    if (keepSelection) {
        tf->selectionEnd = tf->cursorIndex;
    } else {
        textFieldClearSelection(tf);
    }
    textFieldEnsureCursorVisible(tf);
    renderFlag = true;
}

static bool textFieldPixelSelected(TextField *tf, int x, int y) {
    if (!textFieldHasSelection(tf)) return false;

    int start = textFieldSelectionMin(tf);
    int end = textFieldSelectionMax(tf);
    for (int i = start; i < end && i < tf->trackedLen; i++) {
        char c = tf->text[i];
        if (c == '\n') continue;
        int charW = getCharacterWidth(tf->font, c);
        if (charW < 1) charW = 1;
        int sx = tf->contentX + tf->caretX[i] - (int)tf->scrollX;
        int sy = tf->contentY + tf->caretY[i] - (int)tf->scrollY;
        if (x >= sx && x < sx + charW && y >= sy && y < sy + tf->textH) {
            return true;
        }
    }
    return false;
}

static Color textFieldGetPixel(void *self, int x, int y) {
    TextField *tf = (TextField *)self;
    int r = 8;
    int w = tf->w;
    int h = tf->h;

    // Rounded rect mask
    if (x < r && y < r) {
        int dx = x - r;
        int dy = y - r;
        if (dx*dx + dy*dy > r*r) return COLOR_TRANSPARENT;
    }
    if (x >= w - r && y < r) {
        int dx = x - (w - r - 1);
        int dy = y - r;
        if (dx*dx + dy*dy > r*r) return COLOR_TRANSPARENT;
    }
    if (x < r && y >= h - r) {
        int dx = x - r;
        int dy = y - (h - r - 1);
        if (dx*dx + dy*dy > r*r) return COLOR_TRANSPARENT;
    }
    if (x >= w - r && y >= h - r) {
        int dx = x - (w - r - 1);
        int dy = y - (h - r - 1);
        if (dx*dx + dy*dy > r*r) return COLOR_TRANSPARENT;
    }

    if (tf->placeholder && tf->placeholder[0]) {
        int headerH = textFieldHeaderHeight(tf);
        if (y < headerH) {
            int titleY = TEXTFIELD_TITLE_GAP;
            int titleX = TEXTFIELD_PADDING_X + (tf->contentW - tf->placeholderW) / 2;
            if (tf->placeholderW > tf->contentW) {
                titleX = TEXTFIELD_PADDING_X - textFieldPlaceholderOffset(tf);
            }
            if (tf->placeholderBitmap &&
                x >= titleX && x < titleX + tf->placeholderW &&
                y >= titleY && y < titleY + tf->placeholderH &&
                tf->placeholderBitmap[(y - titleY) * tf->placeholderW + (x - titleX)]) {
                return M3_OUTLINE;
            }
            return tf->bgColor;
        }
    }

    bool selected = textFieldPixelSelected(tf, x, y);
    if (selected) {
        Color selectionColor = M3_PRIMARY;
        selectionColor.transparent = false;
        if (tf->renderMask && tf->renderMask[y * w + x]) {
            return M3_ON_PRIMARY;
        }
        return selectionColor;
    }

    if (tf->renderMask && tf->renderMask[y * w + x]) {
        return tf->textColor;
    }

    if (tf->editingEnabled && isFrameFocused(tf->frameId) &&
        !textFieldHasSelection(tf) && ((current_timestamp_ms() / 500) % 2 == 0)) {
        int cursorX = tf->contentX + tf->caretX[tf->cursorIndex] - (int)tf->scrollX;
        int cursorY = tf->contentY + tf->caretY[tf->cursorIndex] - (int)tf->scrollY;
        if (x >= cursorX && x < cursorX + 2 &&
            y >= cursorY && y < cursorY + tf->textH) {
            return M3_PRIMARY;
        }
    }

    return tf->bgColor;
}

static void textFieldPreRender(void *self) {
    TextField *tf = (TextField *)self;
    Frames[tf->frameId].continuousRender =
        (tf->placeholder && tf->placeholder[0]) ||
        (tf->editingEnabled && isFrameFocused(tf->frameId)) ||
        tf->isTouchDown;
    if (tf->placeholder && tf->placeholder[0]) {
        renderFlag = true;
    }
    if (tf->editingEnabled && tf->isTouchDown && isFrameFocused(tf->frameId)) {
        int tx, ty;
        getTouchState(&tx, &ty);
        int dx = tx - tf->touchStartX;
        int dy = ty - tf->touchStartY;
        if (!tf->longPressActive &&
            current_timestamp_ms() - tf->touchDownAt >= TEXTFIELD_LONG_PRESS_MS &&
            dx * dx + dy * dy <= 100) {
            tf->longPressActive = true;
            textFieldMoveCursorToPoint(tf, tx, ty, false);
        } else if (tf->longPressActive) {
            textFieldMoveCursorToPoint(tf, tx, ty, false);
        }
    }

    if (!tf->dirty && !tf->editingEnabled) return;

    textFieldRebuildLayout(tf);
    size_t maskSize = (size_t)tf->w * (size_t)tf->h;
    tf->renderMask = (unsigned char *)hal_realloc(tf->renderMask, maskSize);
    if (!tf->renderMask) return;
    hal_memset(tf->renderMask, 0, maskSize);

    for (int i = 0; i < tf->trackedLen; i++) {
        char c = tf->text[i];
        if (c == '\n') continue;

        int charW = getCharacterWidth(tf->font, c);
        if (charW < 1) charW = 1;
        const char *pixels = tf->font.char_data[(unsigned char)c];
        if (!pixels) continue;

        int drawX = tf->contentX + tf->caretX[i] - (int)tf->scrollX;
        int drawY = tf->contentY + tf->caretY[i] - (int)tf->scrollY;
        for (int gy = 0; gy < tf->textH; gy++) {
            int py = drawY + gy;
            if (py < tf->contentY || py >= tf->contentY + tf->contentH) continue;
            if (py < 0 || py >= tf->h) continue;
            for (int gx = 0; gx < charW; gx++) {
                int px = drawX + gx;
                if (px < tf->contentX || px >= tf->contentX + tf->contentW) continue;
                if (px < 0 || px >= tf->w) continue;
                if (pixels[gy * charW + gx] != 0) {
                    tf->renderMask[py * tf->w + px] = 255;
                }
            }
        }
    }

    tf->dirty = false;
}

static void textFieldOnClick(void *self) {
    TextField *tf = (TextField *)self;
    if (tf->editingEnabled) return;
    if (tf->onClick) tf->onClick(tf->arg);
}

static void textFieldOnTouch(void *self, bool isDown) {
    TextField *tf = (TextField *)self;
    if (!tf->editingEnabled) return;

    int tx, ty;
    getTouchState(&tx, &ty);
    if (isDown) {
        focusTextField(tf);
        tf->isTouchDown = true;
        tf->longPressActive = false;
        tf->touchDownAt = current_timestamp_ms();
        tf->touchStartX = tx;
        tf->touchStartY = ty;
        renderFlag = true;
        return;
    }

    tf->isTouchDown = false;
    if (!tf->longPressActive) {
        int dx = tx - tf->touchStartX;
        int dy = ty - tf->touchStartY;
        if (dx * dx + dy * dy <= 100) {
            textFieldMoveCursorToPoint(tf, tx, ty, false);
        }
    }
    tf->longPressActive = false;
    renderFlag = true;
}

static void textFieldOnDestroy(void *self) {
    TextField *tf = (TextField *)self;
    clearTextInputTargetForFrame(tf->frameId);
    if (tf->ownsText && tf->text) hal_free(tf->text);
    if (tf->placeholder) hal_free(tf->placeholder);
    if (tf->placeholderBitmap) hal_free(tf->placeholderBitmap);
    if (tf->renderMask) hal_free(tf->renderMask);
    hal_free(tf);
}

TextField* createTextField(int x, int y, int w, int h, const char *text, Font font,
                           Color textColor, Color bgColor, void (*onClick)(void*), void *arg) {
    TextField *tf = (TextField *)hal_malloc(sizeof(TextField));
    tf->x = x;
    tf->y = y;
    tf->w = w;
    tf->h = h;
    tf->text = hal_strdup(text ? text : "");
    tf->maxLen = (int)hal_strlen(tf->text ? tf->text : "") + 32;
    if (tf->maxLen < 32) tf->maxLen = 32;
    tf->font = font;
    tf->textColor = textColor;
    tf->bgColor = bgColor;
    tf->renderMask = NULL;
    tf->textW = 0;
    tf->textH = 0;
    tf->dirty = true;
    tf->ownsText = true;
    tf->editingEnabled = false;
    tf->multiline = false;
    tf->placeholder = NULL;
    tf->placeholderBitmap = NULL;
    tf->placeholderW = 0;
    tf->placeholderH = 0;
    tf->cursorIndex = textFieldLength(tf);
    tf->selectionStart = tf->cursorIndex;
    tf->selectionEnd = tf->cursorIndex;
    tf->scrollX = 0;
    tf->scrollY = 0;
    tf->trackedLen = 0;
    tf->totalTextHeight = 0;
    tf->contentX = 0;
    tf->contentY = 0;
    tf->contentW = 0;
    tf->contentH = 0;
    tf->isTouchDown = false;
    tf->longPressActive = false;
    tf->touchDownAt = 0;
    tf->touchStartX = 0;
    tf->touchStartY = 0;
    tf->onClick = onClick;
    tf->arg = arg;

    tf->frameId = requestFrame(w, h, x, y, tf, textFieldPreRender, textFieldGetPixel, textFieldOnClick, textFieldOnTouch);
    Frames[tf->frameId].onDestroy = textFieldOnDestroy;
    Frames[tf->frameId].onFocus = textFieldOnFocus;
    Frames[tf->frameId].onBlur = textFieldOnBlur;
    return tf;
}

void updateTextField(TextField *tf, const char *text) {
    if (!tf) return;
    if (tf->ownsText) {
        int newLen = (int)hal_strlen(text ? text : "");
        textFieldEnsureCapacity(tf, newLen);
        if (tf->text) hal_free(tf->text);
        tf->text = hal_strdup(text ? text : "");
        tf->maxLen = newLen + 32;
        if (tf->maxLen < 32) tf->maxLen = 32;
    } else if (tf->text) {
        int maxCopy = tf->maxLen > 0 ? tf->maxLen - 1 : 0;
        int i = 0;
        for (; text && text[i] && i < maxCopy; i++) tf->text[i] = text[i];
        if (tf->maxLen > 0) tf->text[i] = '\0';
    }
    tf->cursorIndex = textFieldLength(tf);
    textFieldClearSelection(tf);
    tf->scrollX = 0;
    tf->scrollY = 0;
    tf->dirty = true;
    renderFlag = true;
}

void setTextFieldExternalBuffer(TextField *tf, char *buffer, int maxLen) {
    if (!tf || !buffer || maxLen <= 0) return;
    if (tf->ownsText && tf->text) hal_free(tf->text);
    tf->text = buffer;
    tf->maxLen = maxLen;
    tf->ownsText = false;
    tf->cursorIndex = textFieldLength(tf);
    textFieldClearSelection(tf);
    tf->dirty = true;
    renderFlag = true;
}

void setTextFieldEditingEnabled(TextField *tf, bool enabled) {
    if (!tf) return;
    tf->editingEnabled = enabled;
    Frames[tf->frameId].focusable = enabled;
    Frames[tf->frameId].preservesTextInput = enabled;
    if (!enabled) {
        tf->isTouchDown = false;
        tf->longPressActive = false;
        clearTextInputTargetForFrame(tf->frameId);
    }
    tf->dirty = true;
    renderFlag = true;
}

void setTextFieldMultiline(TextField *tf, bool multiline) {
    if (!tf) return;
    tf->multiline = multiline;
    tf->dirty = true;
    renderFlag = true;
}

void setTextFieldPlaceholder(TextField *tf, const char *placeholder) {
    if (!tf) return;
    if (tf->placeholder) {
        hal_free(tf->placeholder);
        tf->placeholder = NULL;
    }
    if (tf->placeholderBitmap) {
        hal_free(tf->placeholderBitmap);
        tf->placeholderBitmap = NULL;
    }

    tf->placeholder = hal_strdup(placeholder ? placeholder : "");
    if (tf->placeholder && tf->placeholder[0]) {
        tf->placeholderW = getTextWidth(tf->placeholder, tf->font);
        tf->placeholderH = getTextHeight(tf->font);
        tf->placeholderBitmap = getTextBitmap(tf->font, tf->placeholder);
    } else {
        tf->placeholderW = 0;
        tf->placeholderH = 0;
    }
    tf->dirty = true;
    renderFlag = true;
}

void focusTextField(TextField *tf) {
    if (!tf || !tf->editingEnabled) return;
    textFieldActivateSession(tf);
}

void blurTextField(TextField *tf) {
    if (!tf) return;
    clearTextInputTargetForFrame(tf->frameId);
}

bool isTextFieldFocused(TextField *tf) {
    if (!tf) return false;
    return isFrameFocused(tf->frameId);
}

void textFieldInsertText(TextField *tf, const char *text) {
    if (!tf || !text || !text[0]) return;

    textFieldDeleteSelection(tf);
    int len = textFieldLength(tf);
    int insertLen = (int)hal_strlen(text);
    if (tf->ownsText) {
        textFieldEnsureCapacity(tf, len + insertLen);
    }

    int maxInsert = insertLen;
    if (len + maxInsert >= tf->maxLen) {
        maxInsert = tf->maxLen - len - 1;
    }
    if (maxInsert <= 0) return;

    hal_memcpy(tf->text + tf->cursorIndex + maxInsert,
               tf->text + tf->cursorIndex,
               (size_t)(len - tf->cursorIndex + 1));
    hal_memcpy(tf->text + tf->cursorIndex, text, (size_t)maxInsert);
    tf->cursorIndex += maxInsert;
    textFieldClearSelection(tf);
    tf->dirty = true;
    renderFlag = true;
}

void textFieldBackspace(TextField *tf) {
    if (!tf) return;
    if (textFieldHasSelection(tf)) {
        textFieldDeleteSelection(tf);
        return;
    }
    if (tf->cursorIndex <= 0) return;
    textFieldDeleteRange(tf, tf->cursorIndex - 1, tf->cursorIndex);
}

void textFieldSelectAll(TextField *tf) {
    if (!tf) return;
    int len = textFieldLength(tf);
    tf->selectionStart = 0;
    tf->selectionEnd = len;
    tf->cursorIndex = len;
    tf->dirty = true;
    renderFlag = true;
}

void textFieldCopy(TextField *tf) {
    if (!tf || !textFieldHasSelection(tf)) return;
    int start = textFieldSelectionMin(tf);
    int end = textFieldSelectionMax(tf);
    int copyLen = end - start;
    if (copyLen >= TEXTFIELD_MAX_TRACKED) copyLen = TEXTFIELD_MAX_TRACKED - 1;
    for (int i = 0; i < copyLen; i++) {
        textFieldClipboard[i] = tf->text[start + i];
    }
    textFieldClipboard[copyLen] = '\0';
}

void textFieldCut(TextField *tf) {
    if (!tf || !textFieldHasSelection(tf)) return;
    textFieldCopy(tf);
    textFieldDeleteSelection(tf);
}

void textFieldPaste(TextField *tf) {
    if (!tf) return;
    textFieldInsertText(tf, textFieldClipboard);
}

const char *getTextFieldText(TextField *tf) {
    if (!tf) return NULL;
    return tf->text;
}

int getTextFieldFrameId(TextField *tf) {
    if (!tf) return -1;
    return tf->frameId;
}
