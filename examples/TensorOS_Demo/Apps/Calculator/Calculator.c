#include <stdlib.h>
#include <string.h>
#include "Calculator.h"
#include "../../../../hal/str/str.h"
#include "../../../../hal/stdio/stdio.h"
#include "../../../../hal/screen/screen.h"
#include "../../../../TensorUI/VStack/VStack.h"
#include "../../../../TensorUI/Button/Button.h"
#include "../../../../TensorUI/Label/Label.h"
#include "../../../../TensorUI/Color/color.h"
#include "../../../../TensorUI/WindowManager.h"
#include "../AppFonts.h"

typedef struct {
    char display[32];
    double operand1;
    char lastOp;
    bool resetOnNext;
    Label* displayLbl;
    VStack* mainStack;
} CalcState;

static CalcState state;

static void updateDisplay() {
    updateLabel(state.displayLbl, state.display);
    renderFlag = true;
}

static void onDigit(void* arg) {
    char digit = (char)(size_t)arg;
    if (state.resetOnNext) {
        state.display[0] = '\0';
        state.resetOnNext = false;
    }
    if (hal_strlen(state.display) < 15) {
        if (digit == '.') {
            if (strchr(state.display, '.') != NULL) return; // Only one decimal
        }
        char s[2] = {digit, '\0'};
        strcat(state.display, s);
    }
    updateDisplay();
}

static void onOp(void* arg) {
    char op = (char)(size_t)arg;
    state.operand1 = atof(state.display);
    state.lastOp = op;
    state.resetOnNext = true;
}

static void onEquals(void* arg) {
    double operand2 = atof(state.display);
    double result = 0;
    if (state.lastOp == '+') result = state.operand1 + operand2;
    else if (state.lastOp == '-') result = state.operand1 - operand2;
    else if (state.lastOp == '*') result = state.operand1 * operand2;
    else if (state.lastOp == '/') result = (operand2 != 0) ? state.operand1 / operand2 : 0;
    
    hal_snprintf(state.display, sizeof(state.display), "%.4g", result);
    state.resetOnNext = true;
    updateDisplay();
}

static void onClear(void* arg) {
    strcpy(state.display, "0");
    state.resetOnNext = true;
    updateDisplay();
}

static void onCalcDestroy(void* self) {
    vstackOnDestroy(self);
    state.mainStack = NULL;
    state.displayLbl = NULL;
}

void pushCalculatorApp() {
    strcpy(state.display, "0");
    state.resetOnNext = true;

    if (state.mainStack) {
        updateDisplay();
        pushWindow(state.mainStack->frameId);
        return;
    }

    // Calculator uses a fixed layout, no scroll needed
    state.mainStack = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4);
    Frames[state.mainStack->frameId].bgcolor = M3_DARK_BG;
    Frames[state.mainStack->frameId].onDestroy = onCalcDestroy;
    configureFrameAsAppSurface(state.mainStack->frameId, true);
    ScreenInsets insets = getScreenSafeInsets();
    applyFramePaddingInsets(state.mainStack->frameId, insets);

#ifdef ROUND_SCREEN
    int displayHeight = 34;
    int displayTop = 6;
    int buttonW = 40;
    int buttonH = 24;
    int rowSpacing = 4;
    Font displayFont = font_18;
#else
    int displayHeight = 44;
    int displayTop = 6;
    int buttonW = 52;
    int buttonH = 32;
    int rowSpacing = 4;
    Font displayFont = font_22;
#endif
    
    int gridWidth = 4 * buttonW + 3 * rowSpacing;
    int availableWidth = getScreenContentWidth(insets);
    int sidePadding = (availableWidth - gridWidth) / 2;
    if (sidePadding < 0) sidePadding = 0;
    int displayX = insets.left + sidePadding;

    state.displayLbl = createLabel(displayX, 0, gridWidth, displayHeight, "0", M3_ON_SURFACE, M3_SURFACE, displayFont);
    state.displayLbl->alignment = ALIGNMENT_RIGHT | ALIGNMENT_CENTER_VERTICAL;
    Frames[state.displayLbl->frameId].alignment = ALIGNMENT_RIGHT;
    Frames[state.displayLbl->frameId].marginTop = displayTop;
    addFrameToVStack(state.mainStack, state.displayLbl->frameId);


    const char* keys[5][4] = {
        {"C", NULL, NULL, "/"},
        {"7", "8", "9", "*"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"},
        {"0", ".", NULL, "="}
    };

    for (int r = 0; r < 5; r++) {
        HStack* row = createHStack(0, 0, SCREEN_WIDTH, buttonH, rowSpacing);
        Frames[row->frameId].paddingLeft = sidePadding; 
        addFrameToVStack(state.mainStack, row->frameId);
        
        for (int c = 0; c < 4; c++) {
            const char* key = keys[r][c];
            if (key == NULL || key[0] == '\0') {
               // Spacer/Empty slot
               Button *btn = createButton(0, 0, buttonW, buttonH, "", COLOR_TRANSPARENT, COLOR_TRANSPARENT, COLOR_TRANSPARENT, font_18, NULL, NULL);
               addFrameToHStack(row, btn->frameId);
               continue;
            }

            Color currentBg = M3_SURFACE;
            void (*cb)(void*) = NULL;
            void* cbArg = NULL;

            if ((key[0] >= '0' && key[0] <= '9') || key[0] == '.') {
                cb = onDigit;
                cbArg = (void*)(size_t)key[0];
                currentBg = M3_SURFACE;
            } else if (key[0] == '=') {
                cb = onEquals;
                currentBg = M3_PRIMARY;
            } else if (key[0] == 'C') {
                cb = onClear;
                currentBg = M3_ERROR;
            } else {
                cb = onOp;
                cbArg = (void*)(size_t)key[0];
                currentBg = M3_SURFACE_VARIANT;
            }
            
            Button *btn = createButton(0, 0, buttonW, buttonH, (char*)key, M3_ON_SURFACE, currentBg, COLOR_WHITE, font_18, cb, cbArg);
            setButtonCornerRadius(btn, 8); // Rounded square buttons
            addFrameToHStack(row, btn->frameId);
        }
    }

    pushWindow(state.mainStack->frameId);
}
