#include "WindowManager.h"
#include "../hal/stdio/stdio.h"
#include "../hal/time/time.h"
#include "Animation/Tween.h"
#include <stdlib.h>


extern Frame Frames[];
extern bool renderFlag;

WindowManager WinMgr;
static bool isAnimatingOut = false;
static int animatingFrameId = -1;
static bool isSwipingBack = false;
static bool isHomeScaling = false;
static int savedFocusFrameIds[MAX_WINDOWS];
static TextInputTarget savedTextTargets[MAX_WINDOWS];

static void logWindowStack(const char *tag) {
    (void)tag;
}

static bool isFrameWithinWindow(int frameId, int windowRootId) {
    int current = frameId;
    while (current != -1) {
        if (current == windowRootId) return true;
        current = Frames[current].parentId;
    }
    return false;
}

static int findWindowSlot(int frameId) {
    for (int i = 0; i < WinMgr.count; i++) {
        if (WinMgr.frameIds[i] == frameId) return i;
    }
    return -1;
}

static void clearSavedInputStateAt(int index) {
    if (index < 0 || index >= MAX_WINDOWS) return;
    savedFocusFrameIds[index] = -1;
    savedTextTargets[index] = (TextInputTarget){ .frameId = -1 };
}

static void saveInputStateForWindow(int windowRootId) {
    int slot = findWindowSlot(windowRootId);
    if (slot == -1) return;

    clearSavedInputStateAt(slot);
    if (focus >= 0 && isFrameWithinWindow(focus, windowRootId)) {
        savedFocusFrameIds[slot] = focus;
    }
    if (currentTextInputTarget.frameId >= 0 &&
        isFrameWithinWindow(currentTextInputTarget.frameId, windowRootId)) {
        savedTextTargets[slot] = currentTextInputTarget;
    }
}

static void restoreInputStateForWindow(int windowRootId) {
    int slot = findWindowSlot(windowRootId);
    if (slot == -1) return;

    TextInputTarget savedTarget = savedTextTargets[slot];
    int savedFocus = savedFocusFrameIds[slot];
    if (savedTarget.frameId >= 0 &&
        savedTarget.frameId < MAX_FRAMES &&
        Frames[savedTarget.frameId].enabled &&
        isFrameWithinWindow(savedTarget.frameId, windowRootId)) {
        registerTextInputTarget(savedTarget);
    } else if (savedFocus >= 0 &&
               savedFocus < MAX_FRAMES &&
               Frames[savedFocus].enabled &&
               isFrameWithinWindow(savedFocus, windowRootId)) {
        setFocusedFrame(savedFocus);
    }
}

static void finalizePopWindow(int topId) {
    // 1. Logically remove BEFORE physical destruction to avoid re-entrancy
    int topIndex = -1;
    for (int i = 0; i < WinMgr.count; i++) {
        if (WinMgr.frameIds[i] == topId) {
            topIndex = i;
            break;
        }
    }

    if (topIndex != -1) {
        // Shift remaining (though usually it's just the top)
        for (int i = topIndex; i < WinMgr.count - 1; i++) {
            WinMgr.frameIds[i] = WinMgr.frameIds[i+1];
            savedFocusFrameIds[i] = savedFocusFrameIds[i+1];
            savedTextTargets[i] = savedTextTargets[i+1];
        }
        WinMgr.count--;
        if (WinMgr.count < MAX_WINDOWS) {
            WinMgr.frameIds[WinMgr.count] = -1;
            clearSavedInputStateAt(WinMgr.count);
        }
    }

    // 2. Trigger physical recursive destruction
    destroyFrame(topId);

    if (WinMgr.count > 0) {
        int nextId = WinMgr.frameIds[WinMgr.count - 1];
        Frames[nextId].enabled = true;
        Frames[nextId].isPaused = false;
        setFrameBufferEnabled(nextId, true);
        if (Frames[nextId].onResume) Frames[nextId].onResume(Frames[nextId].object);
        restoreInputStateForWindow(nextId);
        createTween(&Frames[nextId].fx, 0.0f, 200, EASE_OUT_QUAD);
    }

    isAnimatingOut = false;
    animatingFrameId = -1;
    isSwipingBack = false;
    invalidateFullScreen();
}

void initWindowManager() {
    WinMgr.count = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        WinMgr.frameIds[i] = -1;
        clearSavedInputStateAt(i);
    }
    logWindowStack("init");
}

extern void setFrameBufferEnabled(int id, bool enabled);

void pushWindow(int frameId) {
    if (isAnimatingOut || isHomeScaling) return; // Ignore during exit animations
    if (frameId < 0 || frameId >= MAX_FRAMES || !Frames[frameId].enabled) return;

    int existingSlot = findWindowSlot(frameId);
    if (existingSlot != -1 && existingSlot == WinMgr.count - 1) {
        return;
    }
    if (existingSlot != -1) {
        // Reusing an already stacked window would duplicate lifecycle state and
        // can lead to double-destruction later, so ignore the second push.
        return;
    }

    if (WinMgr.count < MAX_WINDOWS) {
        // Disable previous top if exists
        if (WinMgr.count > 0) {
            int prevId = WinMgr.frameIds[WinMgr.count - 1];
            cancelAllFrameInteractions();
            saveInputStateForWindow(prevId);
            clearTextInputTarget();
            clearFocusedFrame();
            if (Frames[prevId].onPause) Frames[prevId].onPause(Frames[prevId].object);
            Frames[prevId].isPaused = true; // Freeze it
            setFrameBufferEnabled(WinMgr.frameIds[WinMgr.count - 1], false);
        }
        
        WinMgr.frameIds[WinMgr.count++] = frameId;
        Frames[frameId].enabled = true; // Ensure frame is logically enabled
        setFrameBufferEnabled(frameId, true);
        Frames[frameId].isPaused = false;
        
        // Initialize app
        if (Frames[frameId].onCreate) Frames[frameId].onCreate(Frames[frameId].object);

        // Slide from right
        Frames[frameId].fx = (float)SCREEN_WIDTH;
        createTween(&Frames[frameId].fx, 0.0f, 300, EASE_OUT_QUAD);
        
        invalidateFullScreen();
        logWindowStack("push");
    }
}


void popWindow() {
    logWindowStack("pop-request");
    if (isAnimatingOut || isHomeScaling) return; // Prevent double trigger
    
    if (WinMgr.count > 1) { 
        int topId = WinMgr.frameIds[WinMgr.count - 1];
        cancelAllFrameInteractions();
        clearTextInputTarget();
        clearFocusedFrame();
        
        // INTERCEPT CLOSING: Regular pages can be blocked
        if (Frames[topId].onCloseRequest) {
            if (!Frames[topId].onCloseRequest(Frames[topId].object)) {
                // If callback returns false, snap back to 0 and don't pop
                createTween(&Frames[topId].fx, 0.0f, 250, EASE_OUT_QUAD);
                invalidateFullScreen();
                return; 
            }
        }
        // PREPARE BACKGROUND FOR ANIMATION
        if (WinMgr.count > 1) {
            int prevId = WinMgr.frameIds[WinMgr.count - 2];
            Frames[prevId].enabled = true;
            Frames[prevId].isPaused = false;
            setFrameBufferEnabled(prevId, true);
        }
        finalizePopWindow(topId);
    }
}

void popToHome() {
    logWindowStack("home-request");
    if (WinMgr.count <= 1) return;
    if (isHomeScaling) return;
    cancelAllFrameInteractions();
    clearTextInputTarget();
    clearFocusedFrame();

    // PRIVILEGED: Clear any ongoing regular pop animations to avoid double-free
    isAnimatingOut = false;
    isSwipingBack = false;

    int topId = WinMgr.frameIds[WinMgr.count - 1];
    Frames[topId].fx = 0; // Ensure it doesn't trigger fx-based slide logic
    isHomeScaling = true;
    animatingFrameId = topId;
    
    // PREPARE HOME SCREEN FOR REVEAL
    int homeId = WinMgr.frameIds[0];
    Frames[homeId].enabled = true;
    Frames[homeId].isPaused = false;
    setFrameBufferEnabled(homeId, true);
    if (Frames[homeId].onResume) Frames[homeId].onResume(Frames[homeId].object);

    // Zoom out / Shrink animation
    createTween(&Frames[topId].fscale, 0.001f, 350, EASE_OUT_QUAD);
    invalidateFullScreen();
}





int getTopWindow() {
    if (WinMgr.count > 0) return WinMgr.frameIds[WinMgr.count - 1];
    return -1;
}

bool canBeginSystemBackGesture() {
    bool allowed = WinMgr.count > 1 && !isAnimatingOut && !isHomeScaling;
    return allowed;
}

void updateWindowManager() {
    if (WinMgr.count <= 1 && !isAnimatingOut) return; 
    
    int mx, my;
    getTouchState(&mx, &my);
    
    // 1. Handle Active Swipe
    if (currentInputSession.dragging && currentInputSession.gestureIntent == GESTURE_SYSTEM_BACK) {
        int dx = mx - currentInputSession.startX;
        int dy = my - currentInputSession.startY;
        if (dx > 8 && abs(dx) > abs(dy)) {
            int topId = getTopWindow();
            int homeId = (WinMgr.count > 0) ? WinMgr.frameIds[0] : -1;
            if (topId != -1 && topId != homeId) {
                if (!isSwipingBack) {
                    isSwipingBack = true;
                    // Prepare background
                    if (WinMgr.count > 1) {
                        int prevId = WinMgr.frameIds[WinMgr.count - 2];
                        Frames[prevId].enabled = true;
                        Frames[prevId].isPaused = false;
                        setFrameBufferEnabled(prevId, true);
                    }
                }
                Frames[topId].fx = (float)dx;
                invalidateFullScreen();
            }
        }
    } 
    // 2. Handle Release
    else if (isSwipingBack && !currentInputSession.dragging) {
        isSwipingBack = false;
        int dx = mx - currentInputSession.startX;
        int topId = getTopWindow();
        
        if (topId != -1) {
            int completeThreshold = (int)(SCREEN_WIDTH * 0.16f);
            if (completeThreshold < 36) completeThreshold = 36;
            if (dx > completeThreshold) {
                // Check if we are allowed to close
                bool canClose = true;
                if (Frames[topId].onCloseRequest) {
                    canClose = Frames[topId].onCloseRequest(Frames[topId].object);
                }
                if (canClose) {
                    // POP: Animate the rest of the way
                    isAnimatingOut = true;
                    animatingFrameId = topId;
                    createTween(&Frames[topId].fx, (float)SCREEN_WIDTH, 200, EASE_OUT_QUAD);
                } else {
                    // INTERCEPTED: Snap back to 0
                    createTween(&Frames[topId].fx, 0.0f, 250, EASE_OUT_QUAD);
                }
            } else {
                // CANCEL: Snap back to 0
                createTween(&Frames[topId].fx, 0.0f, 250, EASE_OUT_QUAD);
            }
            invalidateFullScreen();
        }
    }

    // 3. Monitor Exit Animation
    if (isAnimatingOut && animatingFrameId != -1) {
        if (Frames[animatingFrameId].fx >= SCREEN_WIDTH - 1) {
            finalizePopWindow(animatingFrameId);
        }
    }
    
    // 4. Monitor Home Scaling Animation
    if (isHomeScaling && animatingFrameId != -1) {
        if (Frames[animatingFrameId].fscale <= 0.05f) {
            isHomeScaling = false;
            // Clear entire stack except index 0
            while (WinMgr.count > 1) {
                int id = WinMgr.frameIds[WinMgr.count - 1];
                WinMgr.frameIds[WinMgr.count - 1] = -1; // Null logic slot
                destroyFrame(id);
                WinMgr.count--;
            }
            int homeId = WinMgr.frameIds[0];
            Frames[homeId].enabled = true;
            Frames[homeId].isPaused = false;
            setFrameBufferEnabled(homeId, true);
            Frames[homeId].fscale = 1.0f; // Ensure home is visible
            if (Frames[homeId].onResume) Frames[homeId].onResume(Frames[homeId].object);
            restoreInputStateForWindow(homeId);
            
            animatingFrameId = -1;
            invalidateFullScreen();
        }
    }
}
