#include "Keyboard.h"
#include "../../hal/mem/mem.h"
#include "../../hal/str/str.h"
#include "../../hal/screen/screen.h"
#include "../WindowManager.h"

#define KEY_COUNT 31
#define LABEL_SIZE 8

typedef struct {
    int x;
    int y;
    int w;
    int h;
    char label[LABEL_SIZE];
    bool primary;
    bool secondary;
    bool *bitmap;
    int textW;
    int textH;
} KeyboardKey;

struct Keyboard {
    int frameId;
    int width;
    int height;
    bool shift;
    bool isRound;
    int mode; // 0 alpha, 1 sym1, 2 sym2
    int pressedKey;
    char *buffer;
    int maxLen;
    TextField *targetField;
    void (*onDone)(void *);
    void *doneArg;
    Font font;
    KeyboardKey keys[KEY_COUNT];
};

static void keyboardRefreshLabels(Keyboard *kb);
static void keyboardBuildLayout(Keyboard *kb);
static void keyboardUpdateBitmap(Keyboard *kb, int index);

static void keyboardSetLabel(Keyboard *kb, int index, const char *label, bool primary, bool secondary) {
    KeyboardKey *key = &kb->keys[index];
    hal_strncpy(key->label, label, LABEL_SIZE - 1);
    key->label[LABEL_SIZE - 1] = '\0';
    key->primary = primary;
    key->secondary = secondary;
    keyboardUpdateBitmap(kb, index);
}

static void keyboardUpdateBitmap(Keyboard *kb, int index) {
    KeyboardKey *key = &kb->keys[index];
    if (key->bitmap) {
        hal_free(key->bitmap);
        key->bitmap = NULL;
    }
    key->textW = getTextWidth(key->label, kb->font);
    key->textH = getTextHeight(kb->font);
    key->bitmap = getTextBitmap(kb->font, key->label);
}

static Color keyboardKeyColor(Keyboard *kb, int index) {
    KeyboardKey *key = &kb->keys[index];
    if (kb->pressedKey == index) {
        // Highlight logic
        if (key->primary) return M3_SECONDARY; // Shift/Done: turn to secondary color
        if (key->secondary) return M3_SECONDARY; // System: turn to secondary
        return M3_PRIMARY; // Normal key: turn to primary color
    }
    if (key->primary) return M3_PRIMARY;
    if (key->secondary) return M3_SURFACE;
    return M3_SURFACE_VARIANT;
}

static Color keyboardKeyTextColor(Keyboard *kb, int index) {
    KeyboardKey *key = &kb->keys[index];
    if (key->primary) return M3_ON_PRIMARY;
    return M3_ON_SURFACE;
}

static bool pointInRoundedRect(int x, int y, int w, int h, int r) {
    if (x < 0 || y < 0 || x >= w || y >= h) return false;
    if (x >= r && x < w - r) return true;
    if (y >= r && y < h - r) return true;

    int cx = x < r ? r : w - r - 1;
    int cy = y < r ? r : h - r - 1;
    int dx = x - cx;
    int dy = y - cy;
    return dx * dx + dy * dy <= r * r;
}

static Color keyboardGetPixel(void *self, int x, int y) {
    Keyboard *kb = (Keyboard *)self;
    int w = kb->width;
    int h = kb->height;
    int panelRadius = 14;

    if (!pointInRoundedRect(x, y, w, h, panelRadius)) {
        return COLOR_TRANSPARENT;
    }

    for (int i = 0; i < KEY_COUNT; i++) {
        KeyboardKey *key = &kb->keys[i];
        int localX = x - key->x;
        int localY = y - key->y;
        if (!pointInRoundedRect(localX, localY, key->w, key->h, 4)) {
            continue;
        }

        int textX = (key->w - key->textW) / 2;
        int textY = (key->h - key->textH) / 2;
        if (key->bitmap &&
            localX >= textX && localX < textX + key->textW &&
            localY >= textY && localY < textY + key->textH &&
            key->bitmap[(localY - textY) * key->textW + (localX - textX)]) {
            return keyboardKeyTextColor(kb, i);
        }

        return keyboardKeyColor(kb, i);
    }

    return M3_SURFACE;
}

static void keyboardBuildLayout(Keyboard *kb) {
    int gap = kb->isRound ? 3 : 4;
    int topPad = 8;
    int bottomPad = 8;
    int rowGap = 5;
    int rowH = (kb->height - topPad - bottomPad - rowGap * 3) / 4;
    int rowY[4] = {
        topPad,
        topPad + rowH + rowGap,
        topPad + (rowH + rowGap) * 2,
        topPad + (rowH + rowGap) * 3
    };

    int rowW[4];
    if (kb->isRound) {
        rowW[0] = kb->width - 24;
        rowW[1] = kb->width - 40;
        rowW[2] = kb->width - 24;
        rowW[3] = kb->width - 12;
    } else {
        rowW[0] = kb->width - 16;
        rowW[1] = kb->width - 28;
        rowW[2] = kb->width - 20;
        rowW[3] = kb->width - 12;
    }

    int rowX0 = (kb->width - rowW[0]) / 2;
    int rowX1 = (kb->width - rowW[1]) / 2;
    int rowX2 = (kb->width - rowW[2]) / 2;
    int rowX3 = (kb->width - rowW[3]) / 2;

    int keyW0 = (rowW[0] - gap * 9) / 10;
    for (int i = 0; i < 10; i++) {
        kb->keys[i].x = rowX0 + i * (keyW0 + gap);
        kb->keys[i].y = rowY[0];
        kb->keys[i].w = keyW0;
        kb->keys[i].h = rowH;
    }

    int keyW1 = (rowW[1] - gap * 8) / 9;
    for (int i = 0; i < 9; i++) {
        kb->keys[10 + i].x = rowX1 + i * (keyW1 + gap);
        kb->keys[10 + i].y = rowY[1];
        kb->keys[10 + i].w = keyW1;
        kb->keys[10 + i].h = rowH;
    }

    int sideW = kb->isRound ? 34 : 38;
    int midW = (rowW[2] - sideW * 2 - gap * 8) / 7;
    kb->keys[19].x = rowX2;
    kb->keys[19].y = rowY[2];
    kb->keys[19].w = sideW;
    kb->keys[19].h = rowH;
    for (int i = 0; i < 7; i++) {
        kb->keys[20 + i].x = rowX2 + sideW + gap + i * (midW + gap);
        kb->keys[20 + i].y = rowY[2];
        kb->keys[20 + i].w = midW;
        kb->keys[20 + i].h = rowH;
    }
    kb->keys[27].x = rowX2 + rowW[2] - sideW;
    kb->keys[27].y = rowY[2];
    kb->keys[27].w = sideW;
    kb->keys[27].h = rowH;

    int symW = kb->isRound ? 36 : 40;
    int doneW = kb->isRound ? 48 : 54;
    int spaceW = rowW[3] - symW - doneW - gap * 2;
    kb->keys[28].x = rowX3;
    kb->keys[28].y = rowY[3];
    kb->keys[28].w = symW;
    kb->keys[28].h = rowH;
    kb->keys[29].x = rowX3 + symW + gap;
    kb->keys[29].y = rowY[3];
    kb->keys[29].w = spaceW;
    kb->keys[29].h = rowH;
    kb->keys[30].x = rowX3 + symW + gap + spaceW + gap;
    kb->keys[30].y = rowY[3];
    kb->keys[30].w = doneW;
    kb->keys[30].h = rowH;
}

static void keyboardPreRender(void *self) {
    Keyboard *kb = (Keyboard *)self;
    kb->width = Frames[kb->frameId].width;
    kb->height = Frames[kb->frameId].height;
    keyboardBuildLayout(kb);
}

static void keyboardActivateLabel(Keyboard *kb, const char *label) {
    if (hal_strcmp(label, "Shift") == 0) {
        kb->shift = !kb->shift;
        keyboardRefreshLabels(kb);
        return;
    }
    if (hal_strcmp(label, "More") == 0) {
        kb->mode = (kb->mode == 1) ? 2 : 1;
        keyboardRefreshLabels(kb);
        return;
    }
    if (hal_strcmp(label, "Sym") == 0) {
        kb->mode = 1;
        kb->shift = false;
        keyboardRefreshLabels(kb);
        return;
    }
    if (hal_strcmp(label, "ABC") == 0) {
        kb->mode = 0;
        kb->shift = false;
        keyboardRefreshLabels(kb);
        return;
    }
    if (hal_strcmp(label, "Back") == 0) {
        if (hasTextInputTarget()) {
            textInputBackspace();
        } else if (kb->targetField) {
            textFieldBackspace(kb->targetField);
        } else {
            int len = (int)hal_strlen(kb->buffer);
            if (len > 0) kb->buffer[len - 1] = '\0';
        }
        return;
    }
    if (hal_strcmp(label, "Space") == 0) {
        if (hasTextInputTarget()) {
            textInputInsertText(" ");
        } else if (kb->targetField) {
            textFieldInsertText(kb->targetField, " ");
        } else {
            int len = (int)hal_strlen(kb->buffer);
            if (len < kb->maxLen - 1) {
                kb->buffer[len] = ' ';
                kb->buffer[len + 1] = '\0';
            }
        }
        return;
    }
    if (hal_strcmp(label, "Done") == 0) {
        if (kb->onDone) {
            kb->onDone(kb->doneArg);
        } else {
            clearTextInputTarget();
            clearFocusedFrame();
            if (WinMgr.count > 0) {
                popWindow();
            }
        }
        return;
    }

    if (hasTextInputTarget()) {
        char s[2] = {label[0], '\0'};
        textInputInsertText(s);
    } else if (kb->targetField) {
        char s[2] = {label[0], '\0'};
        textFieldInsertText(kb->targetField, s);
    } else {
        int len = (int)hal_strlen(kb->buffer);
        if (len >= kb->maxLen - 1) return;
        kb->buffer[len] = label[0];
        kb->buffer[len + 1] = '\0';
    }
}

static int keyboardHitTest(Keyboard *kb, int px, int py) {
    int localX = px - Frames[kb->frameId].x;
    int localY = py - Frames[kb->frameId].y;
    for (int i = 0; i < KEY_COUNT; i++) {
        KeyboardKey *key = &kb->keys[i];
        if (pointInRoundedRect(localX - key->x, localY - key->y, key->w, key->h, 4)) {
            return i;
        }
    }
    return -1;
}

static void keyboardOnClick(void *self) {
    Keyboard *kb = (Keyboard *)self;
    int tx, ty;
    getTouchState(&tx, &ty);
    int hit = keyboardHitTest(kb, tx, ty);
    if (hit >= 0) {
        keyboardActivateLabel(kb, kb->keys[hit].label);
    }
    kb->pressedKey = -1;
    renderFlag = true;
}

static void keyboardOnTouch(void *self, bool isDown) {
    Keyboard *kb = (Keyboard *)self;
    if (!isDown) {
        kb->pressedKey = -1;
        renderFlag = true;
        return;
    }

    int tx, ty;
    getTouchState(&tx, &ty);
    kb->pressedKey = keyboardHitTest(kb, tx, ty);
    renderFlag = true;
}

Keyboard* createKeyboard(int x, int y, int w, int h, bool isRoundScreen, char *buffer, int maxLen, Font font) {
    Keyboard *kb = (Keyboard *)hal_malloc(sizeof(Keyboard));
    kb->frameId = requestFrame(w, h, x, y, kb, keyboardPreRender, keyboardGetPixel, keyboardOnClick, keyboardOnTouch);
    kb->width = w;
    kb->height = h;
    kb->shift = false;
    kb->isRound = isRoundScreen;
    kb->mode = 0;
    kb->pressedKey = -1;
    kb->buffer = buffer;
    kb->maxLen = maxLen;
    kb->targetField = NULL;
    kb->onDone = NULL;
    kb->doneArg = NULL;
    kb->font = font;

    for (int i = 0; i < KEY_COUNT; i++) {
        kb->keys[i].bitmap = NULL;
        kb->keys[i].label[0] = '\0';
        kb->keys[i].primary = false;
        kb->keys[i].secondary = false;
        kb->keys[i].textW = 0;
        kb->keys[i].textH = 0;
    }

    Frames[kb->frameId].preservesTextInput = true;
    keyboardRefreshLabels(kb);
    keyboardBuildLayout(kb);
    return kb;
}

void setKeyboardTarget(Keyboard *kb, char *buffer, int maxLen) {
    if (!kb) return;
    kb->buffer = buffer;
    kb->maxLen = maxLen;
    kb->targetField = NULL;
}

void setKeyboardTextFieldTarget(Keyboard *kb, TextField *tf) {
    if (!kb) return;
    kb->targetField = tf;
}

void setKeyboardRoundScreen(Keyboard *kb, bool isRoundScreen) {
    if (!kb) return;
    kb->isRound = isRoundScreen;
}

void setKeyboardDoneAction(Keyboard *kb, void (*onDone)(void *), void *arg) {
    if (!kb) return;
    kb->onDone = onDone;
    kb->doneArg = arg;
}

int getKeyboardFrameId(Keyboard *kb) {
    if (!kb) return -1;
    return kb->frameId;
}

static void keyboardRefreshLabels(Keyboard *kb) {
    const char *row1Alpha = "qwertyuiop";
    const char *row2Alpha = "asdfghjkl";
    const char *row3Alpha = "zxcvbnm";

    const char *row1Sym1[] = {"1","2","3","4","5","6","7","8","9","0"};
    const char *row2Sym1[] = {"@","#","$","%","&","*","-","+","="};
    const char *row3Sym1[] = {"_","(",")","/","?","!",".",",",":"};

    const char *row1Sym2[] = {"!","@","#","$","%","^","&","*","(",")"};
    const char *row2Sym2[] = {"[","]","{","}","\\","|",";","'","\""};
    const char *row3Sym2[] = {"`","~","<",">","/","?",".",",",":"};

    for (int i = 0; i < 10; i++) {
        if (kb->mode == 0) {
            char s[2] = {row1Alpha[i], 0};
            if (kb->shift) s[0] = (char)(s[0] - 'a' + 'A');
            keyboardSetLabel(kb, i, s, false, false);
        } else if (kb->mode == 1) {
            keyboardSetLabel(kb, i, row1Sym1[i], false, false);
        } else {
            keyboardSetLabel(kb, i, row1Sym2[i], false, false);
        }
    }

    for (int i = 0; i < 9; i++) {
        int idx = 10 + i;
        if (kb->mode == 0) {
            char s[2] = {row2Alpha[i], 0};
            if (kb->shift) s[0] = (char)(s[0] - 'a' + 'A');
            keyboardSetLabel(kb, idx, s, false, false);
        } else if (kb->mode == 1) {
            keyboardSetLabel(kb, idx, row2Sym1[i], false, false);
        } else {
            keyboardSetLabel(kb, idx, row2Sym2[i], false, false);
        }
    }

    keyboardSetLabel(kb, 19, kb->mode == 0 ? "Shift" : "More", true, false);
    for (int i = 0; i < 7; i++) {
        int idx = 20 + i;
        if (kb->mode == 0) {
            char s[2] = {row3Alpha[i], 0};
            if (kb->shift) s[0] = (char)(s[0] - 'a' + 'A');
            keyboardSetLabel(kb, idx, s, false, false);
        } else if (kb->mode == 1) {
            keyboardSetLabel(kb, idx, row3Sym1[i], false, false);
        } else {
            keyboardSetLabel(kb, idx, row3Sym2[i], false, false);
        }
    }
    keyboardSetLabel(kb, 27, "Back", false, true);
    keyboardSetLabel(kb, 28, kb->mode == 0 ? "Sym" : "ABC", false, true);
    keyboardSetLabel(kb, 29, "Space", false, false);
    keyboardSetLabel(kb, 30, "Done", true, false);
}
