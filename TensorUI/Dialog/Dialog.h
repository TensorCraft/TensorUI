#ifndef DIALOG_H
#define DIALOG_H

#include "../Color/color.h"
#include "../Font/font.h"
#include "../../hal/screen/screen.h"
#include <stdbool.h>

typedef struct {
    char *title;
    char *message;
    char *positiveText;
    char *negativeText;
    void (*onPositive)(void* arg);
    void (*onNegative)(void* arg);
    void* arg;
    int frameId;
    int overlayFrameId;
    int previousFocusFrameId;
    TextInputTarget previousTextInputTarget;
} Dialog;

Dialog* createDialog(char *title, char *message, char *posText, char *negText, Font tFont, Font mFont, void (*onPos)(void*), void (*onNeg)(void*), void* arg);
void showDialog(Dialog *dialog);
void closeDialog(Dialog *dialog);

#endif
