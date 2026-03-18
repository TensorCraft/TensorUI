#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <stdbool.h>
#include "../hal/screen/screen.h"

#define MAX_WINDOWS 10

typedef struct {
    int frameIds[MAX_WINDOWS];
    int count;
    void (*onBack)(void);
} WindowManager;

void initWindowManager();
void pushWindow(int frameId);
void popWindow();
void popToHome();
int getTopWindow();
void updateWindowManager();
bool canBeginSystemBackGesture();

// Global Window Manager State
extern WindowManager WinMgr;

#endif
