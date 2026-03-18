#ifndef TEXTFIELD_H
#define TEXTFIELD_H

#include "../Color/color.h"
#include "../Font/font.h"
#include <stdbool.h>

typedef struct TextField TextField;

TextField* createTextField(int x, int y, int w, int h, const char *text, Font font,
                           Color textColor, Color bgColor, void (*onClick)(void*), void *arg);
void updateTextField(TextField *tf, const char *text);
void setTextFieldExternalBuffer(TextField *tf, char *buffer, int maxLen);
void setTextFieldEditingEnabled(TextField *tf, bool enabled);
void setTextFieldMultiline(TextField *tf, bool multiline);
void setTextFieldPlaceholder(TextField *tf, const char *placeholder);
void focusTextField(TextField *tf);
void blurTextField(TextField *tf);
bool isTextFieldFocused(TextField *tf);
void textFieldInsertText(TextField *tf, const char *text);
void textFieldBackspace(TextField *tf);
void textFieldSelectAll(TextField *tf);
void textFieldCopy(TextField *tf);
void textFieldCut(TextField *tf);
void textFieldPaste(TextField *tf);
const char *getTextFieldText(TextField *tf);
int getTextFieldFrameId(TextField *tf);

#endif
