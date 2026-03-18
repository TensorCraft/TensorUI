#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../Color/color.h"
#include "../Font/font.h"
#include "../TextField/TextField.h"
#include <stdbool.h>

typedef struct Keyboard Keyboard;

Keyboard* createKeyboard(int x, int y, int w, int h, bool isRoundScreen, char *buffer, int maxLen, Font font);
void setKeyboardTarget(Keyboard *kb, char *buffer, int maxLen);
void setKeyboardTextFieldTarget(Keyboard *kb, TextField *tf);
void setKeyboardRoundScreen(Keyboard *kb, bool isRoundScreen);
void setKeyboardDoneAction(Keyboard *kb, void (*onDone)(void *), void *arg);
int getKeyboardFrameId(Keyboard *kb);

#endif
