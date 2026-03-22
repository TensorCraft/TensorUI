#ifndef SCREEN_H
#define SCREEN_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "../../TensorUI/Color/color.h"
#include "../../config.h"

// Alignment Constants
#define ALIGNMENT_TOP 0
#define ALIGNMENT_BOTTOM 1
#define ALIGNMENT_CENTER_VERTICAL 2
#define ALIGNMENT_LEFT 0
#define ALIGNMENT_RIGHT 4
#define ALIGNMENT_CENTER_HORIZONTAL 8

typedef enum {
    OVERFLOW_VISIBLE,
    OVERFLOW_HIDDEN,
    OVERFLOW_SCROLL
} Overflow;

typedef enum {
    GESTURE_NONE = 0,
    GESTURE_SYSTEM_BACK,
    GESTURE_SYSTEM_HOME,
    GESTURE_SCROLL_VERTICAL,
    GESTURE_SCROLL_HORIZONTAL,
    GESTURE_CANCEL_CLICK
} GestureIntent;

typedef struct {
    bool active;
    bool dragging;
    bool movedSignificantly;
    int startX;
    int startY;
    int currentX;
    int currentY;
    uint32_t startTimeMs;
    int clickCandidateFrameId;
    int ownerFrameId;
    GestureIntent gestureIntent;
} InputSession;

typedef struct {
    int frameId;
    void *context;
    void (*insertText)(void *context, const char *text);
    void (*backspace)(void *context);
    void (*selectAll)(void *context);
    void (*copy)(void *context);
    void (*cut)(void *context);
    void (*paste)(void *context);
} TextInputTarget;

typedef struct {
    int top;
    int right;
    int bottom;
    int left;
} ScreenInsets;

typedef struct {
    bool valid;
    int x;
    int y;
    int width;
    int height;
} DirtyRegion;

typedef struct
{
    int width, height, x, y;
    float fx, fy; // Float coordinates for smooth animation
    float fscale; // Scale factor (1.0 = normal)
    void (*preRender)(void* self);

    Color (*getPixel)(void *self, int x, int y); 
    void (*onClick)(void *self);
    void (*onTouch)(void *self, bool isDown);
    void (*onCancelInteraction)(void *self); // Forced cancel; must not behave like a normal release
    void (*onUpdate)(void *self);  // Called every frame
    
    // Lifecycle Hooks
    void (*onCreate)(void *self);  // Called when frame is created
    void (*onDestroy)(void *self); // Called when frame is destroyed (recursive)
    void (*onPause)(void *self);   // Called when window goes to background
    void (*onResume)(void *self);  // Called when window returns to foreground
    void (*onFocus)(void *self);   // Called when frame gains framework focus
    void (*onBlur)(void *self);    // Called when frame loses framework focus
    
    bool (*onCloseRequest)(void *self); // Return false to block closing
    
    bool enabled;
    bool isPaused;
    bool isWindowRoot; // If true, this frame acts as an off-screen buffer for an app
    bool isApp;        // Privileged level: Top-level application (handles home gestures)
    bool isSystemLayer; // If true, rendered last as an overlay (Toast, Dialog, etc.)
    bool isExternalBuffer; // If true, buffer memory is managed externally
    bool focusable;    // If true, may become the framework-focused frame
    bool preservesTextInput; // If true, pressing it should not blur active text input
    bool continuousRender; // If true, preRender runs every frame until the frame clears it
    Color *buffer;     // The actual pixel buffer for this frame
    
    void *object;
    void *tag;      // Custom user data
    int scrollType; // 0=none, 1=vertical, 2=horizontal
    int paddingTop, paddingBottom, paddingLeft, paddingRight;
    int marginTop, marginBottom, marginLeft, marginRight;
    int alignment;
    int clipX, clipY, clipW, clipH;
    Color bgcolor;
    int parentId; // -1 if no parent
    Overflow overflowX;
    Overflow overflowY;
} Frame;


// Global variables
extern int frameCount;
extern int focus;
extern InputSession currentInputSession;
extern TextInputTarget currentTextInputTarget;
extern bool renderFlag;
extern Frame Frames[MAX_FRAMES];
extern DirtyRegion pendingDirtyRegion;

// Function declarations
void setAntiAliasing(bool enabled);
bool init_screen(const char *title, int width, int height);
ScreenInsets getScreenSafeInsets(void);
int getScreenContentWidth(ScreenInsets insets);
int getScreenContentHeight(ScreenInsets insets);
void applyFramePaddingInsets(int frameId, ScreenInsets insets);
void configureFrameAsSystemSurface(int frameId, bool enabled);
void configureFrameAsAppSurface(int frameId, bool isApp);
void Pixel(int x, int y, Color color);
void fillScreen(Color color);
void fillRect(int x, int y, int w, int h, Color color);
bool updateScreen();
void close_screen();
bool init_touch();
void getTouchState(int *x, int *y);

int requestFrame(int width, int height, int x, int y, void *object, 
                 void (*preRender)(void *self), 
                 Color (*getPixel)(void *self, int x, int y), 
                 void (*onClick)(void *self), 
                 void (*onTouch)(void *self, bool isDown));

void destroyFrame(int id);
void setFrameBufferEnabled(int id, bool enabled);
Color* getFrameBuffer(int id);
void setFocusedFrame(int frameId);
void clearFocusedFrame(void);
bool isFrameFocused(int frameId);
void registerTextInputTarget(TextInputTarget target);
void clearTextInputTarget(void);
void clearTextInputTargetForFrame(int frameId);
bool hasTextInputTarget(void);
void cancelAllFrameInteractions(void);
void textInputInsertText(const char *text);
void textInputBackspace(void);
void textInputSelectAll(void);
void textInputCopy(void);
void textInputCut(void);
void textInputPaste(void);
void invalidateScreenRect(int x, int y, int width, int height);
void invalidateFullScreen(void);
void invalidateFrame(int frameId);
void invalidateFrameRect(int frameId, int x, int y, int width, int height);
DirtyRegion getPendingDirtyRegion(void);
void clearPendingDirtyRegion(void);
void beginRenderTransaction(void);
void endRenderTransaction(void);
void beginAtomicRenderUpdate(void);
void endAtomicRenderUpdate(void);

#endif
