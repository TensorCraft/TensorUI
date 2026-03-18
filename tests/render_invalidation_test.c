#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../hal/screen/screen.h"

bool canBeginSystemBackGesture(void) {
    return false;
}

void popToHome(void) {
}

void getTouchState(int *x, int *y) {
    if (x) *x = 0;
    if (y) *y = 0;
}

static void reset_render_state(void) {
    frameCount = 0;
    focus = -1;
    renderFlag = false;
    currentInputSession = (InputSession){0};
    currentTextInputTarget = (TextInputTarget){ .frameId = -1 };
    pendingDirtyRegion = (DirtyRegion){0};
    memset(Frames, 0, sizeof(Frames));
}

static void test_screen_rect_union(void) {
    reset_render_state();
    invalidateScreenRect(10, 20, 15, 12);
    invalidateScreenRect(18, 24, 20, 10);

    DirtyRegion dirty = getPendingDirtyRegion();
    assert(dirty.valid);
    assert(dirty.x == 10);
    assert(dirty.y == 20);
    assert(dirty.width == 28);
    assert(dirty.height == 14);
    assert(renderFlag);
}

static void test_frame_invalidation_coordinates(void) {
    reset_render_state();
    int frameId = requestFrame(60, 30, 40, 50, NULL, NULL, NULL, NULL, NULL);
    assert(frameId >= 0);

    invalidateFrame(frameId);
    DirtyRegion whole = getPendingDirtyRegion();
    assert(whole.valid);
    assert(whole.x == 40);
    assert(whole.y == 50);
    assert(whole.width == 60);
    assert(whole.height == 30);

    clearPendingDirtyRegion();
    renderFlag = false;

    invalidateFrameRect(frameId, 6, 7, 12, 9);
    DirtyRegion partial = getPendingDirtyRegion();
    assert(partial.valid);
    assert(partial.x == 46);
    assert(partial.y == 57);
    assert(partial.width == 12);
    assert(partial.height == 9);
    assert(renderFlag);
}

static void test_atomic_update_defers_present_flag(void) {
    reset_render_state();
    int frameId = requestFrame(32, 16, 8, 4, NULL, NULL, NULL, NULL, NULL);
    assert(frameId >= 0);

    beginAtomicRenderUpdate();
    invalidateFrameRect(frameId, 4, 2, 8, 6);
    DirtyRegion dirty = getPendingDirtyRegion();
    assert(dirty.valid);
    assert(!renderFlag);

    endAtomicRenderUpdate();
    dirty = getPendingDirtyRegion();
    assert(dirty.valid);
    assert(renderFlag);
    assert(dirty.x == 12);
    assert(dirty.y == 6);
    assert(dirty.width == 8);
    assert(dirty.height == 6);
}

int main(void) {
    test_screen_rect_union();
    test_frame_invalidation_coordinates();
    test_atomic_update_defers_present_flag();
    printf("render_invalidation_test: ok\n");
    return 0;
}
