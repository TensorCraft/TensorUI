#include "WindowManager.h"
#include <stdlib.h>
#include "../hal/time/time.h"

extern Frame Frames[];
extern bool renderFlag;
extern bool isDragging;
extern int startTouchX;

WindowManager WinMgr;

void initWindowManager() {
    WinMgr.count = 0;
}

void pushWindow(int frameId) {
    if (WinMgr.count < MAX_WINDOWS) {
        // Disable previous top if exists
        if (WinMgr.count > 0) {
            Frames[WinMgr.frameIds[WinMgr.count - 1]].enabled = false;
        }
        
        WinMgr.frameIds[WinMgr.count++] = frameId;
        Frames[frameId].enabled = true;
        Frames[frameId].x = 0; // Reset position
        renderFlag = true;
    }
}

void popWindow() {
    if (WinMgr.count > 1) { // Don't pop the home screen (index 0)
        Frames[WinMgr.frameIds[WinMgr.count - 1]].enabled = false;
        WinMgr.count--;
        Frames[WinMgr.frameIds[WinMgr.count - 1]].enabled = true;
        Frames[WinMgr.frameIds[WinMgr.count - 1]].x = 0;
        renderFlag = true;
    }
}

int getTopWindow() {
    if (WinMgr.count > 0) return WinMgr.frameIds[WinMgr.count - 1];
    return -1;
}

static bool isSwipingBack = false;

void updateWindowManager() {
    if (WinMgr.count <= 1) return; // No back gesture on home screen
    
    int mx, my;
    getTouchState(&mx, &my);
    
    // Simple edge swipe detection (left to right)
    if (isDragging && startTouchX < 40) {
        int dx = mx - startTouchX;
        if (dx > 20) {
            isSwipingBack = true;
            int topId = getTopWindow();
            if (topId != -1) {
                Frames[topId].x = dx; // Visual feedback: slide the window
                renderFlag = true;
            }
        }
    } else if (isSwipingBack && !isDragging) {
        // Released
        isSwipingBack = false;
        int dx = mx - startTouchX;
        int topId = getTopWindow();
        
        if (dx > 100) {
            popWindow();
        } else if (topId != -1) {
            Frames[topId].x = 0; // Snap back
            renderFlag = true;
        }
    }
}
