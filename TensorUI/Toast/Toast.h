#ifndef TOAST_H
#define TOAST_H

#include "../Color/color.h"
#include "../Font/font.h"
#include <stdbool.h>

void initToastManager(Font font);
void showToast(const char *message, int duration_ms);

#endif
