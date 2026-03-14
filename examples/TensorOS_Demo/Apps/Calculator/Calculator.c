#include "Calculator.h"
#include "../../../../hal/screen/screen.h"
#include "../../../../TensorUI/VStack/VStack.h"
#include "../../../../TensorUI/Button/Button.h"
#include "../../../../TensorUI/Label/Label.h"
#include "../../../../TensorUI/Color/color.h"
#include "../../../../TensorUI/WindowManager.h"
#include "../AppFonts.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    if (strlen(state.display) < 15) {
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
    
    snprintf(state.display, sizeof(state.display), "%.4g", result);
    state.resetOnNext = true;
    updateDisplay();
}

static void onClear(void* arg) {
    strcpy(state.display, "0");
    state.resetOnNext = true;
    updateDisplay();
}

void pushCalculatorApp() {
    strcpy(state.display, "0");
    state.resetOnNext = true;
    
    state.mainStack = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    Frames[state.mainStack->frameId].bgcolor = COLOR_BLACK;
    
    // Display
    state.displayLbl = createLabel(0, 0, SCREEN_WIDTH - 20, 60, "0", COLOR_WHITE, COLOR_DARK_GRAY, font_22);
    state.displayLbl->alignment = ALIGNMENT_RIGHT;
    Frames[state.displayLbl->frameId].alignment = ALIGNMENT_RIGHT;
    Frames[state.displayLbl->frameId].marginTop = 20;
    Frames[state.displayLbl->frameId].marginRight = 10;
    addFrameToVStack(state.mainStack, state.displayLbl->frameId);

    const char* keys[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"C", "0", "=", "+"}
    };

    Color btnBg = (Color){45, 45, 45, false};
    Color btnPress = (Color){70, 70, 70, false};

    for (int r = 0; r < 4; r++) {
        HStack* row = createHStack(0, 0, SCREEN_WIDTH, 50, 5);
        addFrameToVStack(state.mainStack, row->frameId);
        for (int c = 0; c < 4; c++) {
            const char* key = keys[r][c];
            Button* btn;
            Color currentBg = btnBg;
            void (*cb)(void*) = NULL;
            void* cbArg = NULL;

            if (key[0] >= '0' && key[0] <= '9') {
                cb = onDigit;
                cbArg = (void*)(size_t)key[0];
            } else if (key[0] == '=') {
                cb = onEquals;
                currentBg = COLOR_CYAN;
            } else if (key[0] == 'C') {
                cb = onClear;
                currentBg = COLOR_RED;
            } else {
                cb = onOp;
                cbArg = (void*)(size_t)key[0];
                currentBg = COLOR_GRAY;
            }
            
            btn = createButton(0, 0, 50, 45, (char*)key, COLOR_WHITE, currentBg, btnPress, font_18, cb, cbArg);
            addFrameToHStack(row, btn->frameId);
        }
    }

    pushWindow(state.mainStack->frameId);
}
