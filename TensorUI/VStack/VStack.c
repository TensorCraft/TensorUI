#include "VStack.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"
#include "../../hal/time/time.h"
#include "../Animation/Tween.h"
#include "../Theme/Theme.h"

extern Frame Frames[];
extern bool renderFlag;

static float clampStackScroll(float value, float minScroll, float maxScroll) {
  if (value < minScroll) return minScroll;
  if (value > maxScroll) return maxScroll;
  return value;
}

static int applyVerticalScrollDeltaRecursive(int frameId, int delta);
static int applyHorizontalScrollDeltaRecursive(int frameId, int delta);

static int applyVStackScrollDelta(VStack *vs, int delta) {
  if (!vs || delta == 0) return delta;

  float minScroll = vs->height > 0 ? (float)(vs->height - vs->totalHeight) : 0.0f;
  if (minScroll > 0) minScroll = 0;

  float oldScroll = vs->scrollY;
  float newScroll = clampStackScroll(oldScroll + (float)delta, minScroll, 0.0f);
  int consumed = (int)(newScroll - oldScroll);
  vs->scrollY = newScroll;
  if (consumed != 0) {
    vs->lastScrollTime = current_timestamp_ms();
    renderFlag = true;
  }
  return delta - consumed;
}

static int applyHStackScrollDelta(HStack *hs, int delta) {
  if (!hs || delta == 0) return delta;

  float minScroll = hs->width > 0 ? (float)(hs->width - hs->totalWidth) : 0.0f;
  if (minScroll > 0) minScroll = 0;

  float oldScroll = hs->scrollX;
  float newScroll = clampStackScroll(oldScroll + (float)delta, minScroll, 0.0f);
  int consumed = (int)(newScroll - oldScroll);
  hs->scrollX = newScroll;
  if (consumed != 0) {
    hs->lastScrollTime = current_timestamp_ms();
    renderFlag = true;
  }
  return delta - consumed;
}

static int applyVerticalScrollDeltaRecursive(int frameId, int delta) {
  if (frameId < 0 || frameId >= MAX_FRAMES || delta == 0) return delta;

  int remaining = delta;
  if (Frames[frameId].scrollType == 1) {
    remaining = applyVStackScrollDelta((VStack *)Frames[frameId].object, remaining);
  }

  if (remaining != 0 && Frames[frameId].parentId != -1) {
    return applyVerticalScrollDeltaRecursive(Frames[frameId].parentId, remaining);
  }
  return remaining;
}

static int applyHorizontalScrollDeltaRecursive(int frameId, int delta) {
  if (frameId < 0 || frameId >= MAX_FRAMES || delta == 0) return delta;

  int remaining = delta;
  if (Frames[frameId].scrollType == 2) {
    remaining = applyHStackScrollDelta((HStack *)Frames[frameId].object, remaining);
  }

  if (remaining != 0 && Frames[frameId].parentId != -1) {
    return applyHorizontalScrollDeltaRecursive(Frames[frameId].parentId, remaining);
  }
  return remaining;
}

static Color vstackGetPixel(void *self, int x, int y) {
  VStack *vs = (VStack *)self;
  if (!vs->showScrollbar)
    return COLOR_TRANSPARENT;
  if (vs->totalHeight <= vs->height)
    return COLOR_TRANSPARENT;

  // Auto-hide: check if more than 1.5s since last scroll
  long long now = current_timestamp_ms();
  if (now - vs->lastScrollTime > 1500)
    return COLOR_TRANSPARENT;

  int sw = 4;
  if (x >= vs->width - sw - 2 && x < vs->width - 2) {
    float visibleRatio = (float)vs->height / vs->totalHeight;
    int barH = (int)(vs->height * visibleRatio);
    if (barH < 20)
      barH = 20;

    float scrollRatio = (float)(-vs->scrollY) / (vs->totalHeight - vs->height);
    int barY = (int)((vs->height - barH) * scrollRatio);

    if (y >= barY && y < barY + barH) {
      return vs->scrollbarColor;
    }
  }
  return COLOR_TRANSPARENT;
}
static void vstackOnTouch(void *self, bool isDown) {
  VStack *vs = (VStack *)self;
  vs->isDragging = isDown;
  Frames[vs->frameId].continuousRender = true;
  if (isDown) {
    int mx, my;
    getTouchState(&mx, &my);
    vs->lastMouseY = my;
    vs->lastScrollTime = current_timestamp_ms();
  }
  renderFlag = true;
}

void cancelVStackInteraction(VStack *vs) {
  if (!vs) return;
  vs->isDragging = false;
  Frames[vs->frameId].continuousRender = false;
  renderFlag = true;
}

static void vstackCancelInteractionHook(void *self) {
  cancelVStackInteraction((VStack *)self);
}

static void vstackOnPause(void *self) {
  VStack *vs = (VStack *)self;
  cancelVStackInteraction(vs);
}

void vstackOnDestroy(void *self) {
  VStack *vs = (VStack *)self;
  if (vs->childFrameIds)
    hal_free(vs->childFrameIds);
  hal_free(vs);
}

static void vstackPreRender(void *self) {
  VStack *vs = (VStack *)self;
  Frame *vsFrame = &Frames[vs->frameId];
  vs->x = vsFrame->x;
  vs->y = vsFrame->y;

  int totalH = vsFrame->paddingTop;
  for (int i = 0; i < vs->childCount; i++) {
    int id = vs->childFrameIds[i];
    totalH +=
        Frames[id].marginTop + Frames[id].height + Frames[id].marginBottom;
    if (i < vs->childCount - 1)
      totalH += vs->spacing;
  }
  totalH += vsFrame->paddingBottom;
  vs->totalHeight = totalH;

  if (vs->isDragging) {
    int mx, my;
    getTouchState(&mx, &my);
    int delta = my - vs->lastMouseY;
    if (delta != 0) {
      applyVerticalScrollDeltaRecursive(vs->frameId, delta);
      vs->lastMouseY = my;
    } else {
      // Even if no movement, while holding we might want to keep it visible?
      // Let's just update time while dragging
      vs->lastScrollTime = current_timestamp_ms();
      renderFlag = true;
    }
  }

  float currentY = (float)vs->y + vs->scrollY;
  int freeSpaceY = vs->height - vs->totalHeight;
  if ((vsFrame->alignment & ALIGNMENT_CENTER_VERTICAL) && freeSpaceY > 0) {
    currentY += (float)(freeSpaceY / 2 + vsFrame->paddingTop);
  } else if ((vsFrame->alignment & ALIGNMENT_BOTTOM) && freeSpaceY > 0) {
    currentY += (float)(freeSpaceY + vsFrame->paddingTop);
  } else {
    currentY += (float)vsFrame->paddingTop;
  }

  for (int i = 0; i < vs->childCount; i++) {
    int id = vs->childFrameIds[i];
    currentY += (float)Frames[id].marginTop;
    Frames[id].y = (int)currentY;
    if (vsFrame->alignment & ALIGNMENT_CENTER_HORIZONTAL) {
      Frames[id].x = vs->x + (vs->width - Frames[id].width) / 2;
    } else if (vsFrame->alignment & ALIGNMENT_RIGHT) {
      Frames[id].x = vs->x + vs->width - vsFrame->paddingRight -
                     Frames[id].marginRight - Frames[id].width;
    } else {
      Frames[id].x = vs->x + vsFrame->paddingLeft + Frames[id].marginLeft;
    }
    Frames[id].fx = (float)Frames[id].x;
    Frames[id].fy = (float)Frames[id].y;

    // Centralized compositor now handles overflow/clipping
    currentY +=
        (float)(Frames[id].height + Frames[id].marginBottom + vs->spacing);
  }

  // Check for fade-out to trigger a re-render if it was visible
  long long now = current_timestamp_ms();
  Frames[vs->frameId].continuousRender =
      vs->isDragging || (vs->showScrollbar && (now - vs->lastScrollTime < 2000));
  if (vs->showScrollbar && (now - vs->lastScrollTime < 2000)) {
    renderFlag = true;
  }
}

VStack *createVStack(int x, int y, int w, int h, int spacing) {
  VStack *vs = (VStack *)hal_malloc(sizeof(VStack));
  vs->x = x;
  vs->y = y;
  vs->width = w;
  vs->height = h;
  vs->scrollY = 0;
  vs->spacing = spacing;
  vs->totalHeight = 0;
  vs->maxChildren = 32;
  vs->childFrameIds = (int *)hal_malloc(sizeof(int) * vs->maxChildren);
  vs->childCount = 0;
  vs->isDragging = false;
  vs->showScrollbar = false;
  vs->scrollbarColor = M3_OUTLINE;
  vs->lastScrollTime = 0;
  vs->frameId = requestFrame(w, h, x, y, vs, vstackPreRender, vstackGetPixel,
                             NULL, vstackOnTouch);
  if (vs->frameId == -1) {
    hal_free(vs->childFrameIds);
    hal_free(vs);
    return NULL;
  }
  Frames[vs->frameId].scrollType = 1;
  Frames[vs->frameId].overflowX = OVERFLOW_HIDDEN;
  Frames[vs->frameId].overflowY = OVERFLOW_HIDDEN;
  Frames[vs->frameId].onDestroy = vstackOnDestroy;
  Frames[vs->frameId].onPause = vstackOnPause;
  Frames[vs->frameId].onCancelInteraction = vstackCancelInteractionHook;
  return vs;
}

void addFrameToVStack(VStack *vs, int id) {
  if (vs->childCount < vs->maxChildren) {
    vs->childFrameIds[vs->childCount++] = id;
    Frames[id].parentId = vs->frameId;
    renderFlag = true;
  }
}

void setVStackContentInsets(VStack *vs, int top, int right, int bottom, int left) {
  if (!vs) return;
  Frames[vs->frameId].paddingTop = top;
  Frames[vs->frameId].paddingRight = right;
  Frames[vs->frameId].paddingBottom = bottom;
  Frames[vs->frameId].paddingLeft = left;
  renderFlag = true;
}

void setVStackScrollbar(VStack *vs, bool visible) {
  vs->showScrollbar = visible;
  renderFlag = true;
}

void setVStackScrollbarColor(VStack *vs, Color c) {
  vs->scrollbarColor = c;
  renderFlag = true;
}

static Color hstackGetPixel(void *self, int x, int y) {
  HStack *hs = (HStack *)self;
  if (!hs->showScrollbar)
    return COLOR_TRANSPARENT;
  if (hs->totalWidth <= hs->width)
    return COLOR_TRANSPARENT;

  long long now = current_timestamp_ms();
  if (now - hs->lastScrollTime > 1500)
    return COLOR_TRANSPARENT;

  int sh = 4;
  if (y >= hs->height - sh - 2 && y < hs->height - 2) {
    float visibleRatio = (float)hs->width / hs->totalWidth;
    int barW = (int)(hs->width * visibleRatio);
    if (barW < 20)
      barW = 20;

    float scrollRatio = (float)(-hs->scrollX) / (hs->totalWidth - hs->width);
    int barX = (int)((hs->width - barW) * scrollRatio);

    if (x >= barX && x < barX + barW) {
      return hs->scrollbarColor;
    }
  }
  return COLOR_TRANSPARENT;
}
static void hstackOnTouch(void *self, bool isDown) {
  HStack *hs = (HStack *)self;
  bool wasDragging = hs->isDragging;
  hs->isDragging = isDown;
  Frames[hs->frameId].continuousRender = true;
  if (isDown) {
    int mx, my;
    getTouchState(&mx, &my);
    hs->lastMouseX = mx;
    hs->lastScrollTime = current_timestamp_ms();
  } else if (wasDragging && hs->paging) {
    // Handle Paging Snap on release
    float targetX = 0;
    int page = (int)((-hs->scrollX + hs->width / 2.0f) / hs->width);
    targetX = -(float)(page * hs->width);

    float minScroll = (float)(hs->width - hs->totalWidth);
    if (minScroll > 0)
      minScroll = 0;
    if (targetX < minScroll)
      targetX = minScroll;
    if (targetX > 0)
      targetX = 0;

    createTween(&hs->scrollX, targetX, 250, EASE_OUT_QUAD);
  }
  renderFlag = true;
}

void cancelHStackInteraction(HStack *hs) {
  if (!hs) return;
  hs->isDragging = false;
  Frames[hs->frameId].continuousRender = false;
  renderFlag = true;
}

static void hstackCancelInteractionHook(void *self) {
  cancelHStackInteraction((HStack *)self);
}

static void hstackOnPause(void *self) {
  HStack *hs = (HStack *)self;
  cancelHStackInteraction(hs);
}

void hstackOnDestroy(void *self) {
  HStack *hs = (HStack *)self;
  if (hs->childFrameIds)
    hal_free(hs->childFrameIds);
  hal_free(hs);
}

static void hstackPreRender(void *self) {
  HStack *hs = (HStack *)self;
  Frame *hsFrame = &Frames[hs->frameId];
  hs->x = hsFrame->x;
  hs->y = hsFrame->y;

  int totalW = hsFrame->paddingLeft;
  for (int i = 0; i < hs->childCount; i++) {
    int id = hs->childFrameIds[i];
    totalW += Frames[id].marginLeft + Frames[id].width + Frames[id].marginRight;
    if (i < hs->childCount - 1)
      totalW += hs->spacing;
  }
  totalW += hsFrame->paddingRight;
  hs->totalWidth = totalW;

  if (hs->isDragging) {
    int mx, my;
    getTouchState(&mx, &my);
    int delta = mx - hs->lastMouseX;
    if (delta != 0) {
      applyHorizontalScrollDeltaRecursive(hs->frameId, delta);
      hs->lastMouseX = mx;
    } else {
      hs->lastScrollTime = current_timestamp_ms();
      renderFlag = true;
    }
  }

  float currentX = (float)hs->x + hs->scrollX;
  int freeSpaceX = hs->width - hs->totalWidth;
  if ((hsFrame->alignment & ALIGNMENT_CENTER_HORIZONTAL) && freeSpaceX > 0) {
    currentX += (float)(freeSpaceX / 2 + hsFrame->paddingLeft);
  } else if ((hsFrame->alignment & ALIGNMENT_RIGHT) && freeSpaceX > 0) {
    currentX += (float)(freeSpaceX + hsFrame->paddingLeft);
  } else {
    currentX += (float)hsFrame->paddingLeft;
  }

  for (int i = 0; i < hs->childCount; i++) {
    int id = hs->childFrameIds[i];
    currentX += (float)Frames[id].marginLeft;
    Frames[id].x = (int)currentX;
    if (hsFrame->alignment & ALIGNMENT_CENTER_VERTICAL) {
      Frames[id].y = hs->y + (hs->height - Frames[id].height) / 2;
    } else if (hsFrame->alignment & ALIGNMENT_BOTTOM) {
      Frames[id].y = hs->y + hs->height - hsFrame->paddingBottom -
                     Frames[id].marginBottom - Frames[id].height;
    } else {
      Frames[id].y = hs->y + hsFrame->paddingTop + Frames[id].marginTop;
    }
    Frames[id].fx = (float)Frames[id].x;
    Frames[id].fy = (float)Frames[id].y;

    // Centralized compositor now handles overflow/clipping
    currentX +=
        (float)(Frames[id].width + Frames[id].marginRight + hs->spacing);
  }

  long long now = current_timestamp_ms();
  Frames[hs->frameId].continuousRender =
      hs->isDragging || (hs->showScrollbar && (now - hs->lastScrollTime < 2000));
  if (hs->showScrollbar && (now - hs->lastScrollTime < 2000)) {
    renderFlag = true;
  }
}

HStack *createHStack(int x, int y, int w, int h, int spacing) {
  HStack *hs = (HStack *)hal_malloc(sizeof(HStack));
  hs->x = x;
  hs->y = y;
  hs->width = w;
  hs->height = h;
  hs->scrollX = 0;
  hs->spacing = spacing;
  hs->totalWidth = 0;
  hs->maxChildren = 32;
  hs->childFrameIds = (int *)hal_malloc(sizeof(int) * hs->maxChildren);
  hs->childCount = 0;
  hs->isDragging = false;
  hs->showScrollbar = false;
  hs->scrollbarColor = M3_OUTLINE;
  hs->lastScrollTime = 0;
  hs->paging = false;
  hs->frameId = requestFrame(w, h, x, y, hs, hstackPreRender, hstackGetPixel,
                             NULL, hstackOnTouch);
  if (hs->frameId == -1) {
    hal_free(hs->childFrameIds);
    hal_free(hs);
    return NULL;
  }

  Frames[hs->frameId].scrollType = 2;
  Frames[hs->frameId].overflowX = OVERFLOW_HIDDEN;
  Frames[hs->frameId].overflowY = OVERFLOW_HIDDEN;
  Frames[hs->frameId].onDestroy = hstackOnDestroy;
  Frames[hs->frameId].onPause = hstackOnPause;
  Frames[hs->frameId].onCancelInteraction = hstackCancelInteractionHook;
  return hs;
}

void addFrameToHStack(HStack *hs, int id) {
  if (hs->childCount < hs->maxChildren) {
    hs->childFrameIds[hs->childCount++] = id;
    Frames[id].parentId = hs->frameId;
    renderFlag = true;
  }
}

void setHStackContentInsets(HStack *hs, int top, int right, int bottom, int left) {
  if (!hs) return;
  Frames[hs->frameId].paddingTop = top;
  Frames[hs->frameId].paddingRight = right;
  Frames[hs->frameId].paddingBottom = bottom;
  Frames[hs->frameId].paddingLeft = left;
  renderFlag = true;
}

void setHStackScrollbar(HStack *hs, bool visible) {
  hs->showScrollbar = visible;
  renderFlag = true;
}

void setHStackScrollbarColor(HStack *hs, Color c) {
  hs->scrollbarColor = c;
  renderFlag = true;
}

void setHStackPaging(HStack *hs, bool paging) { hs->paging = paging; }
