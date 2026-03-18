#include "screen.h"
#include "../../TensorUI/Color/color.h"
#include "../time/time.h"
#include "../mem/mem.h"
#include <stdint.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
static SDL_Texture *screenTexture = NULL;
static uint32_t *screenPixels = NULL;
typedef struct {
    bool valid;
    int x;
    int y;
    int width;
    int height;
} FrameBoundsSnapshot;

static FrameBoundsSnapshot presentedBounds[MAX_FRAMES];
int frameCount = 0;
int focus = -1;
InputSession currentInputSession = {0};
TextInputTarget currentTextInputTarget = {0};
bool renderFlag = true;
Frame Frames[MAX_FRAMES];
DirtyRegion pendingDirtyRegion = {0};
static int renderTransactionDepth = 0;

bool isFrameVisible(int id);
static void lockInputGestureFromMotion(int currentX, int currentY);
bool canBeginSystemBackGesture(void);
static void renderToScreenRecursiveClipped(int id, DirtyRegion clip);
static bool shouldForceFullscreenComposite(DirtyRegion dirty);
static DirtyRegion clipChildRegionToParent(int childId, int parentId, DirtyRegion clip);

#define BACK_EDGE_ZONE_PX 72
#define BACK_LOCK_DX_PX 6
#define HOME_EDGE_ZONE_PX 44
#define HOME_LOCK_DY_PX 8
#define HOME_COMPLETE_DY_PX 44
#define HOME_FAST_FLICK_MS 560

static DirtyRegion makeDirtyRegion(int x, int y, int width, int height) {
    DirtyRegion region = {0};
    if (width <= 0 || height <= 0) return region;

    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return region;
    if (x + width > SCREEN_WIDTH) width = SCREEN_WIDTH - x;
    if (y + height > SCREEN_HEIGHT) height = SCREEN_HEIGHT - y;
    if (width <= 0 || height <= 0) return region;

    region.valid = true;
    region.x = x;
    region.y = y;
    region.width = width;
    region.height = height;
    return region;
}

static DirtyRegion intersectDirtyRegion(DirtyRegion a, DirtyRegion b) {
    if (!a.valid || !b.valid) return (DirtyRegion){0};

    int x1 = a.x > b.x ? a.x : b.x;
    int y1 = a.y > b.y ? a.y : b.y;
    int x2 = (a.x + a.width) < (b.x + b.width) ? (a.x + a.width) : (b.x + b.width);
    int y2 = (a.y + a.height) < (b.y + b.height) ? (a.y + a.height) : (b.y + b.height);
    return makeDirtyRegion(x1, y1, x2 - x1, y2 - y1);
}

static void unionDirtyRegion(DirtyRegion *target, DirtyRegion region) {
    if (!region.valid) return;
    if (!target->valid) {
        *target = region;
        return;
    }

    int x1 = target->x < region.x ? target->x : region.x;
    int y1 = target->y < region.y ? target->y : region.y;
    int x2 = (target->x + target->width) > (region.x + region.width)
                 ? (target->x + target->width)
                 : (region.x + region.width);
    int y2 = (target->y + target->height) > (region.y + region.height)
                 ? (target->y + target->height)
                 : (region.y + region.height);
    *target = makeDirtyRegion(x1, y1, x2 - x1, y2 - y1);
}

static DirtyRegion makeFrameRegion(int frameId, int x, int y, int width, int height) {
    if (frameId < 0 || frameId >= MAX_FRAMES || !Frames[frameId].enabled) {
        return (DirtyRegion){0};
    }

    int frameWidth = Frames[frameId].width;
    int frameHeight = Frames[frameId].height;
    if (width <= 0) width = frameWidth;
    if (height <= 0) height = frameHeight;
    return makeDirtyRegion(Frames[frameId].x + x, Frames[frameId].y + y, width, height);
}

static void markDirtyRegion(DirtyRegion region) {
    unionDirtyRegion(&pendingDirtyRegion, region);
    if (renderTransactionDepth == 0 && region.valid) {
        renderFlag = true;
    }
}

static DirtyRegion snapshotToDirtyRegion(FrameBoundsSnapshot snapshot) {
    if (!snapshot.valid) return (DirtyRegion){0};
    return makeDirtyRegion(snapshot.x, snapshot.y, snapshot.width, snapshot.height);
}

static FrameBoundsSnapshot captureFrameBoundsSnapshot(int frameId) {
    FrameBoundsSnapshot snapshot = {0};
    if (frameId < 0 || frameId >= MAX_FRAMES || !Frames[frameId].enabled) {
        return snapshot;
    }

    snapshot.valid = true;
    snapshot.x = Frames[frameId].x;
    snapshot.y = Frames[frameId].y;
    snapshot.width = Frames[frameId].width;
    snapshot.height = Frames[frameId].height;

    if (Frames[frameId].isWindowRoot && Frames[frameId].fscale > 0.0f &&
        (Frames[frameId].fscale < 0.99f || Frames[frameId].fscale > 1.01f)) {
        int sw = (int)(Frames[frameId].width * Frames[frameId].fscale);
        int sh = (int)(Frames[frameId].height * Frames[frameId].fscale);
        snapshot.x = Frames[frameId].x + (Frames[frameId].width - sw) / 2;
        snapshot.y = Frames[frameId].y + (Frames[frameId].height - sh) / 2;
        snapshot.width = sw > 0 ? sw : 1;
        snapshot.height = sh > 0 ? sh : 1;
    }

    return snapshot;
}

static bool snapshotsEqual(FrameBoundsSnapshot a, FrameBoundsSnapshot b) {
    return a.valid == b.valid &&
           a.x == b.x &&
           a.y == b.y &&
           a.width == b.width &&
           a.height == b.height;
}

static void invalidateGeometryChanges(void) {
    for (int i = 0; i < MAX_FRAMES; i++) {
        FrameBoundsSnapshot previous = presentedBounds[i];
        FrameBoundsSnapshot current = captureFrameBoundsSnapshot(i);
        if (snapshotsEqual(previous, current)) {
            continue;
        }

        if ((previous.valid || current.valid) &&
            Frames[i].parentId == -1 &&
            (Frames[i].isWindowRoot || Frames[i].isApp || Frames[i].isSystemLayer)) {
            invalidateFullScreen();
            continue;
        }

        markDirtyRegion(snapshotToDirtyRegion(previous));
        markDirtyRegion(snapshotToDirtyRegion(current));
    }
}

static void refreshPresentedBoundsSnapshots(void) {
    for (int i = 0; i < MAX_FRAMES; i++) {
        presentedBounds[i] = captureFrameBoundsSnapshot(i);
    }
}

static uint32_t packScreenPixel(Color color) {
    return ((uint32_t)color.r << 24) |
           ((uint32_t)color.g << 16) |
           ((uint32_t)color.b << 8) |
           0xFFu;
}

ScreenInsets getScreenSafeInsets(void) {
#ifdef ROUND_SCREEN
    return (ScreenInsets){
        .top = 20,
        .right = 18,
        .bottom = 24,
        .left = 18,
    };
#else
    return (ScreenInsets){
        .top = 12,
        .right = 12,
        .bottom = 16,
        .left = 12,
    };
#endif
}

int getScreenContentWidth(ScreenInsets insets) {
    return SCREEN_WIDTH - insets.left - insets.right;
}

int getScreenContentHeight(ScreenInsets insets) {
    return SCREEN_HEIGHT - insets.top - insets.bottom;
}

void applyFramePaddingInsets(int frameId, ScreenInsets insets) {
    if (frameId < 0 || frameId >= MAX_FRAMES) return;
    Frames[frameId].paddingTop = insets.top;
    Frames[frameId].paddingRight = insets.right;
    Frames[frameId].paddingBottom = insets.bottom;
    Frames[frameId].paddingLeft = insets.left;
}

void configureFrameAsSystemSurface(int frameId, bool enabled) {
    if (frameId < 0 || frameId >= MAX_FRAMES) return;
    Frames[frameId].isSystemLayer = enabled;
    if (enabled) {
        Frames[frameId].isApp = false;
    }
}

void configureFrameAsAppSurface(int frameId, bool isApp) {
    if (frameId < 0 || frameId >= MAX_FRAMES) return;
    Frames[frameId].isApp = isApp;
    if (isApp) {
        Frames[frameId].isSystemLayer = false;
    }
}

void invalidateScreenRect(int x, int y, int width, int height) {
    markDirtyRegion(makeDirtyRegion(x, y, width, height));
}

void invalidateFullScreen(void) {
    pendingDirtyRegion = makeDirtyRegion(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (renderTransactionDepth == 0) {
        renderFlag = true;
    }
}

void invalidateFrame(int frameId) {
    markDirtyRegion(makeFrameRegion(frameId, 0, 0, 0, 0));
}

void invalidateFrameRect(int frameId, int x, int y, int width, int height) {
    markDirtyRegion(makeFrameRegion(frameId, x, y, width, height));
}

DirtyRegion getPendingDirtyRegion(void) {
    return pendingDirtyRegion;
}

void clearPendingDirtyRegion(void) {
    pendingDirtyRegion = (DirtyRegion){0};
}

void beginRenderTransaction(void) {
    renderTransactionDepth++;
}

void endRenderTransaction(void) {
    if (renderTransactionDepth <= 0) return;
    renderTransactionDepth--;
    if (renderTransactionDepth == 0 && pendingDirtyRegion.valid) {
        renderFlag = true;
    }
}

void beginAtomicRenderUpdate(void) {
    beginRenderTransaction();
}

void endAtomicRenderUpdate(void) {
    endRenderTransaction();
}

static void resetInputSession(void) {
    currentInputSession = (InputSession){
        .active = false,
        .dragging = false,
        .movedSignificantly = false,
        .startX = 0,
        .startY = 0,
        .currentX = 0,
        .currentY = 0,
        .startTimeMs = 0,
        .clickCandidateFrameId = -1,
        .ownerFrameId = -1,
        .gestureIntent = GESTURE_NONE,
    };
}

void setFocusedFrame(int frameId) {
    if (frameId == focus) return;

    int previousFocus = focus;
    if (previousFocus >= 0 && previousFocus < MAX_FRAMES &&
        Frames[previousFocus].enabled && Frames[previousFocus].onBlur) {
        Frames[previousFocus].onBlur(Frames[previousFocus].object);
    }

    if (frameId >= 0 && frameId < MAX_FRAMES && Frames[frameId].enabled && Frames[frameId].focusable) {
        focus = frameId;
        if (Frames[frameId].onFocus) {
            Frames[frameId].onFocus(Frames[frameId].object);
        }
        return;
    }

    focus = -1;
}

void clearFocusedFrame(void) {
    setFocusedFrame(-1);
}

bool isFrameFocused(int frameId) {
    return focus == frameId;
}

void registerTextInputTarget(TextInputTarget target) {
    currentTextInputTarget = target;
    if (target.frameId >= 0) {
        setFocusedFrame(target.frameId);
    }
}

void clearTextInputTarget(void) {
    currentTextInputTarget = (TextInputTarget){ .frameId = -1 };
}

void clearTextInputTargetForFrame(int frameId) {
    if (currentTextInputTarget.frameId == frameId) {
        clearTextInputTarget();
    }
    if (focus == frameId) {
        clearFocusedFrame();
    }
}

bool hasTextInputTarget(void) {
    return currentTextInputTarget.frameId >= 0 && currentTextInputTarget.context != NULL;
}

void textInputInsertText(const char *text) {
    if (hasTextInputTarget() && currentTextInputTarget.insertText) {
        currentTextInputTarget.insertText(currentTextInputTarget.context, text);
    }
}

void textInputBackspace(void) {
    if (hasTextInputTarget() && currentTextInputTarget.backspace) {
        currentTextInputTarget.backspace(currentTextInputTarget.context);
    }
}

void textInputSelectAll(void) {
    if (hasTextInputTarget() && currentTextInputTarget.selectAll) {
        currentTextInputTarget.selectAll(currentTextInputTarget.context);
    }
}

void textInputCopy(void) {
    if (hasTextInputTarget() && currentTextInputTarget.copy) {
        currentTextInputTarget.copy(currentTextInputTarget.context);
    }
}

void textInputCut(void) {
    if (hasTextInputTarget() && currentTextInputTarget.cut) {
        currentTextInputTarget.cut(currentTextInputTarget.context);
    }
}

void textInputPaste(void) {
    if (hasTextInputTarget() && currentTextInputTarget.paste) {
        currentTextInputTarget.paste(currentTextInputTarget.context);
    }
}

static void beginInputSession(int x, int y, Uint32 timestamp) {
    currentInputSession = (InputSession){
        .active = true,
        .dragging = false,
        .movedSignificantly = false,
        .startX = x,
        .startY = y,
        .currentX = x,
        .currentY = y,
        .startTimeMs = timestamp,
        .clickCandidateFrameId = -1,
        .ownerFrameId = -1,
        .gestureIntent = GESTURE_NONE,
    };
}

static void releaseInputOwner(void) {
    if (currentInputSession.ownerFrameId != -1 && Frames[currentInputSession.ownerFrameId].onTouch) {
        Frames[currentInputSession.ownerFrameId].onTouch(Frames[currentInputSession.ownerFrameId].object, false);
    }
}

static void cancelFrameInteraction(int frameId) {
    if (frameId < 0 || frameId >= frameCount) return;
    if (!Frames[frameId].enabled) return;

    if (Frames[frameId].onCancelInteraction) {
        Frames[frameId].onCancelInteraction(Frames[frameId].object);
    } else if (Frames[frameId].onTouch) {
        Frames[frameId].onTouch(Frames[frameId].object, false);
    }

    Frames[frameId].continuousRender = false;
}

void cancelAllFrameInteractions(void) {
    for (int i = 0; i < frameCount; i++) {
        cancelFrameInteraction(i);
    }
    resetInputSession();
    SDL_CaptureMouse(SDL_FALSE);
    renderFlag = true;
}

static void completeInputSession(int x, int y, Uint32 timestamp) {
    currentInputSession.currentX = x;
    currentInputSession.currentY = y;
    if (!currentInputSession.active) {
        resetInputSession();
        SDL_CaptureMouse(SDL_FALSE);
        return;
    }

    if (!currentInputSession.movedSignificantly &&
        currentInputSession.clickCandidateFrameId != -1 &&
        Frames[currentInputSession.clickCandidateFrameId].onClick) {
        Frames[currentInputSession.clickCandidateFrameId].onClick(Frames[currentInputSession.clickCandidateFrameId].object);
    }

    int dx = x - currentInputSession.startX;
    int dy = y - currentInputSession.startY;
    Uint32 gestureDurationMs = timestamp - currentInputSession.startTimeMs;
    bool startedNearBottomEdge = currentInputSession.startY > SCREEN_HEIGHT - HOME_EDGE_ZONE_PX;
    bool movedUpFarEnough = dy < -HOME_COMPLETE_DY_PX;
    bool stronglyVertical = abs(dy) * 3 > abs(dx) * 4;
    bool quickFlick = gestureDurationMs <= HOME_FAST_FLICK_MS;
    bool longEnoughSwipe = dy < -(HOME_COMPLETE_DY_PX + 14);
    if (currentInputSession.gestureIntent == GESTURE_SYSTEM_HOME &&
        startedNearBottomEdge && movedUpFarEnough && stronglyVertical &&
        (quickFlick || longEnoughSwipe)) {
        cancelAllFrameInteractions();
        extern void popToHome();
        popToHome();
        return;
    }

    releaseInputOwner();
    resetInputSession();
    SDL_CaptureMouse(SDL_FALSE);
    renderFlag = true;
}

static void updateActiveMotion(int x, int y) {
    if (!currentInputSession.active) return;
    currentInputSession.currentX = x;
    currentInputSession.currentY = y;
    int dx = x - currentInputSession.startX;
    int dy = y - currentInputSession.startY;
    if (dx * dx + dy * dy >= 4 && !currentInputSession.dragging) {
        lockInputGestureFromMotion(x, y);
        renderFlag = true;
    }
}

static bool isLeftMousePressedGlobally(int *windowRelativeX, int *windowRelativeY) {
    int globalX = 0;
    int globalY = 0;
    Uint32 buttons = SDL_GetGlobalMouseState(&globalX, &globalY);
    int windowX = 0;
    int windowY = 0;
    SDL_GetWindowPosition(window, &windowX, &windowY);
    if (windowRelativeX) *windowRelativeX = globalX - windowX;
    if (windowRelativeY) *windowRelativeY = globalY - windowY;
    bool pressed = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    return pressed;
}

static void updateFocusForPress(int frameId) {
    if (frameId == -1) {
        clearTextInputTarget();
        clearFocusedFrame();
        return;
    }

    if (frameId == currentTextInputTarget.frameId || Frames[frameId].preservesTextInput) {
        return;
    }

    clearTextInputTarget();
    if (!Frames[frameId].focusable) {
        clearFocusedFrame();
    }
}

static void setInputOwnerFrame(int frameId, bool notifyDown) {
    currentInputSession.ownerFrameId = frameId;
    if (notifyDown && frameId != -1 && Frames[frameId].onTouch) {
        Frames[frameId].onTouch(Frames[frameId].object, true);
    }
}

static bool pointHitsFrame(int frameId, int x, int y) {
    if (!isFrameVisible(frameId)) return false;
    if (Frames[frameId].clipW != -1 &&
        (x < Frames[frameId].clipX || x >= Frames[frameId].clipX + Frames[frameId].clipW ||
         y < Frames[frameId].clipY || y >= Frames[frameId].clipY + Frames[frameId].clipH)) {
        return false;
    }
    return x >= Frames[frameId].x && x <= Frames[frameId].x + Frames[frameId].width &&
           y >= Frames[frameId].y && y <= Frames[frameId].y + Frames[frameId].height;
}

static void lockInputGestureFromMotion(int currentX, int currentY) {
    int dx = currentX - currentInputSession.startX;
    int dy = currentY - currentInputSession.startY;
    bool horizontalDominant = abs(dx) > abs(dy) + 1;
    bool verticalDominant = abs(dy) > abs(dx) + 2;
    bool backAllowed = canBeginSystemBackGesture();
    bool systemBackCandidate = backAllowed &&
                               currentInputSession.startX < BACK_EDGE_ZONE_PX &&
                               dx > BACK_LOCK_DX_PX && horizontalDominant;
    bool systemHomeCandidate = currentInputSession.startY > SCREEN_HEIGHT - HOME_EDGE_ZONE_PX &&
                               dy < -HOME_LOCK_DY_PX &&
                               (abs(dy) * 3 > abs(dx) * 4 || verticalDominant);

    bool waitingForBackLock = backAllowed &&
                              currentInputSession.startX < BACK_EDGE_ZONE_PX &&
                              dx > 0 &&
                              !systemBackCandidate &&
                              abs(dx) >= abs(dy) &&
                              dx <= BACK_LOCK_DX_PX + 6;
    if (waitingForBackLock) {
        return;
    }

    currentInputSession.dragging = true;
    currentInputSession.movedSignificantly = true;

    if (systemBackCandidate) {
        currentInputSession.gestureIntent = GESTURE_SYSTEM_BACK;
        return;
    }
    if (systemHomeCandidate) {
        currentInputSession.gestureIntent = GESTURE_SYSTEM_HOME;
        return;
    }

    int desiredType = horizontalDominant ? 2 : 1;
    currentInputSession.gestureIntent = horizontalDominant ? GESTURE_SCROLL_HORIZONTAL : GESTURE_SCROLL_VERTICAL;
    for (int i = frameCount - 1; i >= 0; i--) {
        if (!pointHitsFrame(i, currentInputSession.startX, currentInputSession.startY)) continue;
        if (Frames[i].scrollType == desiredType || Frames[i].scrollType == 3) {
            setInputOwnerFrame(i, true);
            return;
        }
    }

    currentInputSession.gestureIntent = GESTURE_CANCEL_CLICK;
}

static DirtyRegion getFrameBoundsRegion(int frameId) {
    return makeDirtyRegion(Frames[frameId].x, Frames[frameId].y, Frames[frameId].width, Frames[frameId].height);
}

static DirtyRegion getFrameClipRegion(int frameId) {
    if (Frames[frameId].clipW != -1 && Frames[frameId].clipH != -1) {
        return makeDirtyRegion(Frames[frameId].clipX, Frames[frameId].clipY, Frames[frameId].clipW, Frames[frameId].clipH);
    }
    return getFrameBoundsRegion(frameId);
}

static DirtyRegion clipChildRegionToParent(int childId, int parentId, DirtyRegion clip) {
    DirtyRegion childClip = intersectDirtyRegion(getFrameClipRegion(childId), clip);
    Frame *parent = &Frames[parentId];
    if (parent->overflowX != OVERFLOW_VISIBLE || parent->overflowY != OVERFLOW_VISIBLE) {
        DirtyRegion parentBounds = getFrameBoundsRegion(parentId);
        if (parent->overflowX == OVERFLOW_VISIBLE) {
            parentBounds.x = childClip.x;
            parentBounds.width = childClip.width;
        }
        if (parent->overflowY == OVERFLOW_VISIBLE) {
            parentBounds.y = childClip.y;
            parentBounds.height = childClip.height;
        }
        childClip = intersectDirtyRegion(childClip, parentBounds);
    }
    return childClip;
}

static void clearRootBufferRegion(int rootId, DirtyRegion clip) {
    if (!Frames[rootId].buffer) return;

    DirtyRegion rootBounds = getFrameBoundsRegion(rootId);
    DirtyRegion visible = intersectDirtyRegion(rootBounds, clip);
    if (!visible.valid) return;

    int startY = visible.y - Frames[rootId].y;
    int endY = startY + visible.height;
    int startX = visible.x - Frames[rootId].x;
    int endX = startX + visible.width;
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            Frames[rootId].buffer[y * Frames[rootId].width + x] = Frames[rootId].bgcolor;
        }
    }
}

static void fillFrameBackgroundIntoRootBufferClipped(int rootId, int frameId, int ox, int oy, DirtyRegion clip) {
    Color bg = Frames[frameId].bgcolor;
    if (bg.transparent) return;

    DirtyRegion frameBounds = getFrameBoundsRegion(frameId);
    DirtyRegion visible = intersectDirtyRegion(frameBounds, clip);
    if (!visible.valid) return;

    int startY = visible.y - Frames[frameId].y;
    int endY = startY + visible.height;
    int startX = visible.x - Frames[frameId].x;
    int endX = startX + visible.width;
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            int lx = (Frames[frameId].x - ox) + x;
            int ly = (Frames[frameId].y - oy) + y;
            if (lx >= 0 && lx < Frames[rootId].width && ly >= 0 && ly < Frames[rootId].height) {
                Frames[rootId].buffer[ly * Frames[rootId].width + lx] = bg;
            }
        }
    }
}

static void renderFrameIntoRootBufferClipped(int rootId, int frameId, int ox, int oy, DirtyRegion clip) {
    if (!Frames[frameId].getPixel) return;

    DirtyRegion frameBounds = getFrameBoundsRegion(frameId);
    DirtyRegion visible = intersectDirtyRegion(frameBounds, clip);
    if (!visible.valid) return;

    int startY = visible.y - Frames[frameId].y;
    int endY = startY + visible.height;
    int startX = visible.x - Frames[frameId].x;
    int endX = startX + visible.width;
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            int lx = (Frames[frameId].x - ox) + x;
            int ly = (Frames[frameId].y - oy) + y;
            if (lx >= 0 && lx < Frames[rootId].width && ly >= 0 && ly < Frames[rootId].height) {
                Color c = Frames[frameId].getPixel(Frames[frameId].object, x, y);
                if (!c.transparent) {
                    Frames[rootId].buffer[ly * Frames[rootId].width + lx] = c;
                }
            }
        }
    }
}

static void renderDescendantsRecursive(int rootId, int parentId, int ox, int oy, DirtyRegion clip) {
    for (int j = 0; j < frameCount; j++) {
        if (Frames[j].enabled && Frames[j].parentId == parentId && Frames[j].getPixel) {
            DirtyRegion childClip = clipChildRegionToParent(j, parentId, clip);
            fillFrameBackgroundIntoRootBufferClipped(rootId, j, ox, oy, childClip);
            renderFrameIntoRootBufferClipped(rootId, j, ox, oy, childClip);
            renderDescendantsRecursive(rootId, j, ox, oy, childClip);
        }
    }
}

static void fillFrameBackgroundToScreenClipped(int frameId, DirtyRegion clip) {
    Color bg = Frames[frameId].bgcolor;
    if (bg.transparent) return;

    DirtyRegion frameBounds = getFrameBoundsRegion(frameId);
    DirtyRegion visible = intersectDirtyRegion(frameBounds, clip);
    if (!visible.valid) return;

    for (int y = visible.y; y < visible.y + visible.height; y++) {
        for (int x = visible.x; x < visible.x + visible.width; x++) {
            Pixel(x, y, bg);
        }
    }
}

static void renderToScreenRecursiveClipped(int id, DirtyRegion clip) {
    if (!Frames[id].enabled) return;

    fillFrameBackgroundToScreenClipped(id, clip);

    if (Frames[id].getPixel) {
        DirtyRegion frameBounds = makeDirtyRegion(Frames[id].x, Frames[id].y, Frames[id].width, Frames[id].height);
        DirtyRegion visible = intersectDirtyRegion(frameBounds, clip);
        if (visible.valid) {
            int startY = visible.y - Frames[id].y;
            int endY = startY + visible.height;
            int startX = visible.x - Frames[id].x;
            int endX = startX + visible.width;
            for (int y = startY; y < endY; y++) {
                for (int x = startX; x < endX; x++) {
                    Pixel(Frames[id].x + x, Frames[id].y + y, Frames[id].getPixel(Frames[id].object, x, y));
                }
            }
        }
    }

    for (int j = 0; j < frameCount; j++) {
        if (Frames[j].enabled && Frames[j].parentId == id) {
            DirtyRegion childClip = clipChildRegionToParent(j, id, clip);
            renderToScreenRecursiveClipped(j, childClip);
        }
    }
}

static bool shouldForceFullscreenComposite(DirtyRegion dirty) {
    if (!dirty.valid) {
        return true;
    }

    if (dirty.x == 0 && dirty.y == 0 &&
        dirty.width == SCREEN_WIDTH && dirty.height == SCREEN_HEIGHT) {
        return false;
    }

    // The SDL simulator keeps a persistent screen pixel buffer between frames.
    // When stacked app surfaces contain transparency, a missed exposed-region
    // invalidation shows stale pixels from the previous page. Prefer correctness
    // over partial-present efficiency in the emulator compositor.
    for (int i = 0; i < frameCount; i++) {
        if (!Frames[i].enabled || Frames[i].parentId != -1) continue;
        if (Frames[i].isWindowRoot || Frames[i].isApp || Frames[i].isSystemLayer) {
            return true;
        }
    }

    return false;
}

// Function declarations
void Pixel(int x, int y, Color color);
static bool antiAliasingEnabled = false;

void setAntiAliasing(bool enabled) {
    antiAliasingEnabled = enabled;
    invalidateFullScreen();
}

#ifdef ROUND_SCREEN
bool roundMask[SCREEN_WIDTH][SCREEN_HEIGHT] = {0};
#endif

bool isFrameVisible(int id) {
    if (id < 0 || id >= MAX_FRAMES) return false;
    if (!Frames[id].enabled) return false;
    if (Frames[id].isPaused) return false;
    if (Frames[id].parentId == -1) return true;
    return isFrameVisible(Frames[id].parentId);
}

__attribute__((weak)) int getSystemUIFrozenWindowId(void) { return -1; }
__attribute__((weak)) bool isSystemUICompositeActive(void) { return false; }

#ifdef ROUND_SCREEN
void precalculateRoundMask()
{
    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    for (int i = 0; i < SCREEN_WIDTH; i++)
    {
        for (int j = 0; j < SCREEN_HEIGHT; j++)
        {
            roundMask[i][j] = (i - centerX) * (i - centerX) + (j - centerY) * (j - centerY) > centerX * centerX;
        }
    }
}
#endif

bool init_touch()
{
    return true;
}

// Moved to input_hal.c
/*
void getTouchState(int *x, int *y)
{
    SDL_GetMouseState(x, y);
}
*/

bool init_screen(const char *title, int width, int height)
{
    frameCount = 0;
    for (int i = 0; i < MAX_FRAMES; i++)
        Frames[i].enabled = false;
    for (int i = 0; i < MAX_FRAMES; i++)
        presentedBounds[i] = (FrameBoundsSnapshot){0};
    resetInputSession();
    clearPendingDirtyRegion();
    invalidateFullScreen();
#ifdef ROUND_SCREEN
    precalculateRoundMask();
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("TensorUI Emulator",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH,
                              SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (!window)
    {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_DestroyWindow(window);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    screenTexture = SDL_CreateTexture(renderer,
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      SCREEN_WIDTH,
                                      SCREEN_HEIGHT);
    if (!screenTexture)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_Quit();
        renderer = NULL;
        window = NULL;
        return false;
    }

    screenPixels = (uint32_t *)hal_malloc((size_t)SCREEN_WIDTH * (size_t)SCREEN_HEIGHT * sizeof(uint32_t));
    if (!screenPixels)
    {
        SDL_DestroyTexture(screenTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        printf("screen buffer allocation failed\n");
        SDL_Quit();
        screenTexture = NULL;
        renderer = NULL;
        window = NULL;
        return false;
    }

    uint32_t clearPixel = packScreenPixel(COLOR_BLACK);
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        screenPixels[i] = clearPixel;
    }

    return true;
}

void fillScreen(Color color)
{
    uint32_t packed = packScreenPixel(color);
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        screenPixels[i] = packed;
    }
}

void Pixel(int x, int y, Color color)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }
    if (color.transparent)
        return;
    screenPixels[y * SCREEN_WIDTH + x] = packScreenPixel(color);
}

void Rect(int x, int y, int w, int h, Color color)
{
    for (int i = x; i <= w; i++)
    {
        Pixel(i, y, color);
        Pixel(i, y + h, color);
    }
    for (int j = y + 1; j < y + h; j++)
    {
        Pixel(x, j, color);
        Pixel(x + w, j, color);
    }
}

void Line(int x0, int y0, int x1, int y1, Color color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        Pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void fillRect(int x, int y, int w, int h, Color color)
{
    for (int i = x; i <= x + w; i++)
        for (int j = y; j <= y + h; j++)
            Pixel(i, j, color);
}

void Circle(int x, int y, int r, Color color)
{
    int dx = 0;
    int dy = r;
    int d = 1 - r;

    while (dx <= dy)
    {
        Pixel(x + dx, y + dy, color);
        Pixel(x - dx, y + dy, color);
        Pixel(x + dx, y - dy, color);
        Pixel(x - dx, y - dy, color);
        Pixel(x + dy, y + dx, color);
        Pixel(x - dy, y + dx, color);
        Pixel(x + dy, y - dx, color);
        Pixel(x - dy, y - dx, color);

        if (d < 0)
        {
            d += 2 * dx + 3;
        }
        else
        {
            d += 2 * (dx - dy) + 5;
            dy--;
        }
        dx++;
    }
}

void fillCircle(int x, int y, int r, Color color)
{
    int dx = 0;
    int dy = r;
    int d = 1 - r;

    while (dx <= dy)
    {
        for (int i = -dx; i <= dx; i++)
        {
            Pixel(x + i, y + dy, color);
            Pixel(x + i, y - dy, color);
        }
        for (int i = -dy; i <= dy; i++)
        {
            Pixel(x + i, y + dx, color);
            Pixel(x + i, y - dx, color);
        }

        if (d < 0)
        {
            d += 2 * dx + 3;
        }
        else
        {
            d += 2 * (dx - dy) + 5;
            dy--;
        }
        dx++;
    }
}

bool updateScreen()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            printf("SDL_QUIT received, closing...\n");
            close_screen();
            return false;
        }
        // ... (Touch input handling - stays the same for now)
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            beginInputSession(event.button.x, event.button.y, event.button.timestamp);
            SDL_CaptureMouse(SDL_TRUE);
            int topHitFrameId = -1;
            for(int i = frameCount - 1; i >= 0; i--) {
                if(pointHitsFrame(i, event.button.x, event.button.y)) {
                    if (topHitFrameId == -1) {
                        topHitFrameId = i;
                    }
                    if (currentInputSession.clickCandidateFrameId == -1 && Frames[i].onClick != NULL) {
                        currentInputSession.clickCandidateFrameId = i;
                    }
                    // Buttons and other direct-touch widgets can react immediately.
                    // Scroll containers should wait until drag intent is known in motion.
                    if (currentInputSession.ownerFrameId == -1 && Frames[i].onTouch && Frames[i].scrollType == 0) {
                        setInputOwnerFrame(i, true);
                    }
                    if (currentInputSession.clickCandidateFrameId != -1) break;
                }
            }
            updateFocusForPress(topHitFrameId);
        }
        if (event.type == SDL_MOUSEMOTION && currentInputSession.active &&
            ((event.motion.state & SDL_BUTTON_LMASK) || SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))) {
            updateActiveMotion(event.motion.x, event.motion.y);
        }
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            completeInputSession(event.button.x, event.button.y, event.button.timestamp);
        }
        if (event.type == SDL_WINDOWEVENT &&
            (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
             event.window.event == SDL_WINDOWEVENT_HIDDEN ||
             event.window.event == SDL_WINDOWEVENT_LEAVE) &&
            currentInputSession.active) {
            int relativeX = currentInputSession.currentX;
            int relativeY = currentInputSession.currentY;
            bool pressed = isLeftMousePressedGlobally(&relativeX, &relativeY);
            if (!pressed) {
                completeInputSession(relativeX, relativeY, SDL_GetTicks());
            }
        }
    }

    if (currentInputSession.active) {
        int relativeX = currentInputSession.currentX;
        int relativeY = currentInputSession.currentY;
        bool pressed = isLeftMousePressedGlobally(&relativeX, &relativeY);
        if (!pressed) {
            completeInputSession(relativeX, relativeY, SDL_GetTicks());
        } else {
            updateActiveMotion(relativeX, relativeY);
        }
    }

    // Lifecycle
    for (int i = 0; i < frameCount; i++) {
        if (!Frames[i].enabled) continue;
        if (Frames[i].parentId == -1) {
            Frames[i].x = (int)Frames[i].fx; 
            Frames[i].y = (int)Frames[i].fy;
        }
        bool currentlyPaused = Frames[i].isPaused;
        int p = Frames[i].parentId;
        while (p != -1) { if (Frames[p].isPaused) { currentlyPaused = true; break; } p = Frames[p].parentId; }
        if (!currentlyPaused && Frames[i].onUpdate) Frames[i].onUpdate(Frames[i].object);
    }

    // Two pre-render passes stabilize intrinsic sizing before container layout.
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < frameCount; i++) {
            if (!Frames[i].enabled) continue;
            if (Frames[i].preRender && (renderFlag || Frames[i].continuousRender)) {
                Frames[i].preRender(Frames[i].object);
            }
        }
    }

    invalidateGeometryChanges();

    if (renderTransactionDepth > 0) return true;
    if (!renderFlag) return true;
    renderFlag = false;
    DirtyRegion dirty = pendingDirtyRegion.valid
                            ? pendingDirtyRegion
                            : makeDirtyRegion(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    clearPendingDirtyRegion();
    if (shouldForceFullscreenComposite(dirty)) {
        dirty = makeDirtyRegion(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    // 1. App Rendering Phase (Inner Buffering)
    for (int i = 0; i < frameCount; i++) {
        if (Frames[i].enabled && Frames[i].isWindowRoot && Frames[i].buffer) {
            DirtyRegion rootBounds = getFrameBoundsRegion(i);
            DirtyRegion rootDirty = intersectDirtyRegion(rootBounds, dirty);
            if (!rootDirty.valid) {
                continue;
            }

            clearRootBufferRegion(i, rootDirty);
            fillFrameBackgroundIntoRootBufferClipped(i, i, Frames[i].x, Frames[i].y, rootDirty);
            renderFrameIntoRootBufferClipped(i, i, Frames[i].x, Frames[i].y, rootDirty);
            renderDescendantsRecursive(i, i, Frames[i].x, Frames[i].y, rootDirty);
        }
    }

    // 2. Compositor Phase (Screen Composition)
    fillRect(dirty.x, dirty.y, dirty.width - 1, dirty.height - 1, COLOR_BLACK);
    
    // 2.1 First pass: Apps and standard UI
    int frozenWindowId = getSystemUIFrozenWindowId();
    bool systemUICompositeActive = isSystemUICompositeActive();
    for (int i = 0; i < frameCount; i++) {
        bool allowFrozenWindow = systemUICompositeActive && i == frozenWindowId;
        if ((isFrameVisible(i) || allowFrozenWindow) && !Frames[i].isSystemLayer) {
            if (Frames[i].isWindowRoot && Frames[i].buffer) {
                // Blit app buffer to screen with SCALING SUPPORT
                float sc = Frames[i].fscale;
                if (sc >= 0.99f && sc <= 1.01f) {
                    DirtyRegion frameBounds = makeDirtyRegion(Frames[i].x, Frames[i].y, Frames[i].width, Frames[i].height);
                    DirtyRegion visible = intersectDirtyRegion(frameBounds, dirty);
                    if (visible.valid) {
                        int startY = visible.y - Frames[i].y;
                        int endY = startY + visible.height;
                        int startX = visible.x - Frames[i].x;
                        int endX = startX + visible.width;
                        for (int y = startY; y < endY; y++) {
                            for (int x = startX; x < endX; x++) {
                                int sx = Frames[i].x + x;
                                int sy = Frames[i].y + y;
                                Pixel(sx, sy, Frames[i].buffer[y * Frames[i].width + x]);
                            }
                        }
                    }
                } else {
                    // Scaling path: Shrink from center
                    int sw = (int)(Frames[i].width * sc);
                    int sh = (int)(Frames[i].height * sc);
                    int sx0 = Frames[i].x + (Frames[i].width - sw) / 2;
                    int sy0 = Frames[i].y + (Frames[i].height - sh) / 2;
                    DirtyRegion scaledBounds = makeDirtyRegion(sx0, sy0, sw, sh);
                    DirtyRegion visible = intersectDirtyRegion(scaledBounds, dirty);

                    if (visible.valid) {
                        int startY = visible.y - sy0;
                        int endY = startY + visible.height;
                        int startX = visible.x - sx0;
                        int endX = startX + visible.width;
                        for (int y = startY; y < endY; y++) {
                            for (int x = startX; x < endX; x++) {
                                int srcX = (int)(x / sc);
                                int srcY = (int)(y / sc);
                                if (srcX >= 0 && srcX < Frames[i].width && srcY >= 0 && srcY < Frames[i].height) {
                                    int dx = sx0 + x;
                                    int dy = sy0 + y;
                                    Pixel(dx, dy, Frames[i].buffer[srcY * Frames[i].width + srcX]);
                                }
                            }
                        }
                    }
                }
            } else if (Frames[i].parentId == -1 && (!Frames[i].isPaused || allowFrozenWindow)) {
                // Skip paused root pages during forward navigation so the covered
                // page does not linger behind the incoming screen.
                renderToScreenRecursiveClipped(i, dirty);
            }
        }
    }

    // 2.2 Second pass: System Layer (Overlays)
    for (int i = 0; i < frameCount; i++) {
        if (isFrameVisible(i) && Frames[i].isSystemLayer) {
            // Overlays are usually direct render or could be buffered, but usually direct for system elements
            renderToScreenRecursiveClipped(i, dirty);
        }
    }

#ifdef ROUND_SCREEN
    for (int i = dirty.x; i < dirty.x + dirty.width; i++) {
        for (int j = dirty.y; j < dirty.y + dirty.height; j++) {
            if (roundMask[i][j]) {
                screenPixels[j * SCREEN_WIDTH + i] = packScreenPixel(COLOR_DARK_GRAY);
            }
        }
    }
#endif
    SDL_Rect dirtyRect = {
        .x = dirty.x,
        .y = dirty.y,
        .w = dirty.width,
        .h = dirty.height,
    };
    SDL_UpdateTexture(screenTexture,
                      &dirtyRect,
                      screenPixels + dirty.y * SCREEN_WIDTH + dirty.x,
                      SCREEN_WIDTH * (int)sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
    refreshPresentedBoundsSnapshots();
    return true;
}

void close_screen()
{
    for (int i = 0; i < frameCount; i++)
        Frames[i] = (Frame){0};
    if (screenPixels)
        hal_free(screenPixels);
    screenPixels = NULL;
    if (screenTexture)
        SDL_DestroyTexture(screenTexture);
    screenTexture = NULL;
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

int requestFrame(int width, int height, int x, int y, void *object, 
                 void (*preRender)(void *self), 
                 Color (*getPixel)(void *self, int x, int y), 
                 void (*onClick)(void *self), 
                 void (*onTouch)(void *self, bool isDown))
{
    int id = -1;
    // Search for an available slot (reuse)
    for (int i = 0; i < MAX_FRAMES; i++) {
        if (!Frames[i].enabled) {
            id = i;
            break;
        }
    }

    if (id == -1)
    {
        return -1;
    }

    Frame *frame = &Frames[id];

    frame->object = object;
    frame->width = width;
    frame->height = height;
    frame->x = x;
    frame->y = y;
    frame->fx = (float)x;
    frame->fy = (float)y;

    frame->preRender = preRender;
    frame->getPixel = getPixel;
    frame->onClick = onClick;
    frame->onTouch = onTouch;
    frame->onCancelInteraction = NULL;
    frame->onUpdate = NULL;
    frame->onCreate = NULL;
    frame->onDestroy = NULL;
    frame->onPause = NULL;
    frame->onResume = NULL;
    frame->onFocus = NULL;
    frame->onBlur = NULL;
    frame->enabled = true;
    frame->isPaused = false;
    frame->isWindowRoot = false;
    frame->isSystemLayer = false;
    frame->buffer = NULL;
    frame->isExternalBuffer = false; 
    frame->focusable = false;
    frame->preservesTextInput = false;
    frame->continuousRender = false;

    frame->tag = NULL;
    frame->scrollType = 0;
    frame->paddingTop = 0; frame->paddingBottom = 0; frame->paddingLeft = 0; frame->paddingRight = 0;
    frame->marginTop = 0; frame->marginBottom = 0; frame->marginLeft = 0; frame->marginRight = 0;
    frame->alignment = ALIGNMENT_LEFT | ALIGNMENT_TOP;

    frame->clipX = 0; frame->clipY = 0; frame->clipW = -1; frame->clipH = -1;
    frame->bgcolor = COLOR_TRANSPARENT;
    frame->parentId = -1;
    frame->overflowX = OVERFLOW_VISIBLE;
    frame->overflowY = OVERFLOW_VISIBLE;
    frame->fscale = 1.0f;
    frame->isApp = false;
  
    if (id >= frameCount) frameCount = id + 1;
    return id;
}

void setFrameBufferExternal(int id, Color *buffer) {
    if (id < 0 || id >= MAX_FRAMES) return;
    if (Frames[id].buffer && !Frames[id].isExternalBuffer) hal_free(Frames[id].buffer);
    Frames[id].buffer = buffer;
    if (buffer) {
        Frames[id].isWindowRoot = true;
        Frames[id].isExternalBuffer = true;
    } else {
        Frames[id].isWindowRoot = false;
        Frames[id].isExternalBuffer = false;
    }
}

void setFrameBufferEnabled(int id, bool enabled) {
    if (id < 0 || id >= MAX_FRAMES) return;
    Frames[id].isWindowRoot = enabled;
    if (enabled && !Frames[id].buffer) {
        Frames[id].buffer = (Color *)hal_malloc(Frames[id].width * Frames[id].height * sizeof(Color));
        if (Frames[id].buffer) {
            Color fill = Frames[id].bgcolor;
            for (int i = 0; i < Frames[id].width * Frames[id].height; i++) {
                Frames[id].buffer[i] = fill;
            }
        }
    } else if (!enabled && Frames[id].buffer) {
        hal_free(Frames[id].buffer);
        Frames[id].buffer = NULL;
    }
}

void destroyFrame(int id)
{
    if (id < 0 || id >= MAX_FRAMES || !Frames[id].enabled)
        return;

    markDirtyRegion(snapshotToDirtyRegion(captureFrameBoundsSnapshot(id)));
    presentedBounds[id] = (FrameBoundsSnapshot){0};

    clearTextInputTargetForFrame(id);

    // 1. Mark as disabled FIRST to avoid recursive double-destruction
    Frames[id].enabled = false;
    Frames[id].parentId = -1;

    // 2. Recursively destroy children first (Bottom-up destruction)
    for (int i = 0; i < frameCount; i++) {
        if (Frames[i].enabled && Frames[i].parentId == id) {
            destroyFrame(i);
        }
    }

    // 3. Call lifecycle hook
    if (Frames[id].onDestroy) Frames[id].onDestroy(Frames[id].object);

    // 4. Free buffer if we own it
    if (Frames[id].buffer && !Frames[id].isExternalBuffer) {
        hal_free(Frames[id].buffer);
    }
    Frames[id].buffer = NULL;
}
