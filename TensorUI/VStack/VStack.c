#include "VStack.h"
#include <stdlib.h>
#include <string.h>
#include "../../hal/screen/screen.h"

extern Frame Frames[];
extern bool renderFlag;

static void vstackOnTouch(void *self, bool isDown) {
    VStack *vs = (VStack *)self;
    vs->isDragging = isDown;
    if (isDown) {
        int mx, my;
        getTouchState(&mx, &my);
        vs->lastMouseY = my;
    }
}

static void vstackPreRender(void *self) {
    VStack *vs = (VStack *)self;
    
    // Sync current stack position from its frame
    vs->x = Frames[vs->frameId].x;
    vs->y = Frames[vs->frameId].y;

    // Calculate total height considering padding, margins, and spacing
    Frame *vsFrame = &Frames[vs->frameId];
// Update internal coordinates from frame in case of animation (e.g. WindowManager slider)
    vs->x = vsFrame->x;
    vs->y = vsFrame->y;

    int totalH = vsFrame->paddingTop;
    for (int i = 0; i < vs->childCount; i++) {
        int id = vs->childFrameIds[i];
        totalH += Frames[id].marginTop + Frames[id].height + Frames[id].marginBottom;
        if (i < vs->childCount - 1) totalH += vs->spacing;
    }
    totalH += vsFrame->paddingBottom;
    vs->totalHeight = totalH;

    if (vs->isDragging) {
        int mx, my;
        getTouchState(&mx, &my);
        int delta = my - vs->lastMouseY;
        if (delta != 0) {
            vs->scrollY += delta;
            
            // Limit bounds so there's no infinite scrolling
            int minScroll = vs->height > 0 ? vs->height - vs->totalHeight : 0;
            if (minScroll > 0) minScroll = 0; // if children are smaller than container, don't scroll
            
            if (vs->scrollY < minScroll) vs->scrollY = minScroll;
            if (vs->scrollY > 0) vs->scrollY = 0;

            vs->lastMouseY = my;
            renderFlag = true;
        }
    }

    // Always update children relative to current y + scroll
    int currentY = vs->y + vs->scrollY;
    int freeSpaceY = vs->height - vs->totalHeight;
    if ((vsFrame->alignment & ALIGNMENT_CENTER_VERTICAL) && freeSpaceY > 0) {
        currentY += freeSpaceY / 2 + vsFrame->paddingTop;
    } else if ((vsFrame->alignment & ALIGNMENT_BOTTOM) && freeSpaceY > 0) {
        currentY += freeSpaceY + vsFrame->paddingTop;
    } else {
        currentY += vsFrame->paddingTop;
    }

    for (int i = 0; i < vs->childCount; i++) {
        int id = vs->childFrameIds[i];
        currentY += Frames[id].marginTop;
        Frames[id].y = currentY;
        
        // Horizontal cross-axis alignment
        if (vsFrame->alignment & ALIGNMENT_CENTER_HORIZONTAL) {
            Frames[id].x = vs->x + (vs->width - Frames[id].width) / 2;
        } else if (vsFrame->alignment & ALIGNMENT_RIGHT) {
            Frames[id].x = vs->x + vs->width - vsFrame->paddingRight - Frames[id].marginRight - Frames[id].width;
        } else {
            // Default ALIGNMENT_LEFT
            Frames[id].x = vs->x + vsFrame->paddingLeft + Frames[id].marginLeft;
        }

        // Apply Clipping Bound
        int clipX = vs->x;
        int clipY = vs->y;
        int clipW = vs->width;
        int clipH = vs->height;
        if (vsFrame->clipW != -1 && vsFrame->clipH != -1) {
            int cx1 = clipX > vsFrame->clipX ? clipX : vsFrame->clipX;
            int cy1 = clipY > vsFrame->clipY ? clipY : vsFrame->clipY;
            int cx2 = (clipX + clipW) < (vsFrame->clipX + vsFrame->clipW) ? (clipX + clipW) : (vsFrame->clipX + vsFrame->clipW);
            int cy2 = (clipY + clipH) < (vsFrame->clipY + vsFrame->clipH) ? (clipY + clipH) : (vsFrame->clipY + vsFrame->clipH);
            clipX = cx1; clipY = cy1;
            clipW = cx2 > cx1 ? cx2 - cx1 : 0;
            clipH = cy2 > cy1 ? cy2 - cy1 : 0;
        }
        Frames[id].clipX = clipX;
        Frames[id].clipY = clipY;
        Frames[id].clipW = clipW;
        Frames[id].clipH = clipH;

        currentY += Frames[id].height + Frames[id].marginBottom + vs->spacing;
    }
}

VStack* createVStack(int x, int y, int w, int h, int spacing) {
    VStack *vs = (VStack *)malloc(sizeof(VStack));
    vs->x = x;
    vs->y = y;
    vs->width = w;
    vs->height = h;
    vs->scrollY = 0;
    vs->spacing = spacing;
    vs->totalHeight = 0;
    vs->maxChildren = 32;
    vs->childFrameIds = (int *)malloc(sizeof(int) * vs->maxChildren);
    vs->childCount = 0;
    vs->isDragging = false;
    vs->frameId = requestFrame(w, h, x, y, vs, vstackPreRender, NULL, NULL, vstackOnTouch);
    Frames[vs->frameId].scrollType = 1;
    return vs;
}

void addFrameToVStack(VStack *vs, int id) {
    if (vs->childCount < vs->maxChildren) {
        vs->childFrameIds[vs->childCount++] = id;
        Frames[id].parentId = vs->frameId;
        renderFlag = true;
    }
}

static void hstackOnTouch(void *self, bool isDown) {
    HStack *hs = (HStack *)self;
    hs->isDragging = isDown;
    if (isDown) {
        int mx, my;
        getTouchState(&mx, &my);
        hs->lastMouseX = mx;
    }
}

static void hstackPreRender(void *self) {
    HStack *hs = (HStack *)self;
    
    // Sync position
    hs->x = Frames[hs->frameId].x;
    hs->y = Frames[hs->frameId].y;

    Frame *hsFrame = &Frames[hs->frameId];
// Update internal coordinates from frame
    hs->x = hsFrame->x;
    hs->y = hsFrame->y;

    int totalW = hsFrame->paddingLeft;
    for (int i = 0; i < hs->childCount; i++) {
        int id = hs->childFrameIds[i];
        totalW += Frames[id].marginLeft + Frames[id].width + Frames[id].marginRight;
        if (i < hs->childCount - 1) totalW += hs->spacing;
    }
    totalW += hsFrame->paddingRight;
    hs->totalWidth = totalW;

    if (hs->isDragging) {
        int mx, my;
        getTouchState(&mx, &my);
        int delta = mx - hs->lastMouseX;
        if (delta != 0) {
            hs->scrollX += delta;
            
            int minScroll = hs->width > 0 ? hs->width - hs->totalWidth : 0;
            if (minScroll > 0) minScroll = 0;
            
            if (hs->scrollX < minScroll) hs->scrollX = minScroll;
            if (hs->scrollX > 0) hs->scrollX = 0;

            hs->lastMouseX = mx;
            renderFlag = true;
        }
    }

    // Update children
    int currentX = hs->x + hs->scrollX;
    int freeSpaceX = hs->width - hs->totalWidth;
    if ((hsFrame->alignment & ALIGNMENT_CENTER_HORIZONTAL) && freeSpaceX > 0) {
        currentX += freeSpaceX / 2 + hsFrame->paddingLeft;
    } else if ((hsFrame->alignment & ALIGNMENT_RIGHT) && freeSpaceX > 0) {
        currentX += freeSpaceX + hsFrame->paddingLeft;
    } else {
        currentX += hsFrame->paddingLeft;
    }

    for (int i = 0; i < hs->childCount; i++) {
        int id = hs->childFrameIds[i];
        currentX += Frames[id].marginLeft;
        Frames[id].x = currentX;
        
        // Vertical cross-axis alignment
        if (hsFrame->alignment & ALIGNMENT_CENTER_VERTICAL) {
            Frames[id].y = hs->y + (hs->height - Frames[id].height) / 2;
        } else if (hsFrame->alignment & ALIGNMENT_BOTTOM) {
            Frames[id].y = hs->y + hs->height - hsFrame->paddingBottom - Frames[id].marginBottom - Frames[id].height;
        } else {
            // Default ALIGNMENT_TOP
            Frames[id].y = hs->y + hsFrame->paddingTop + Frames[id].marginTop;
        }

        // Apply Clipping Bound
        int clipX = hs->x;
        int clipY = hs->y;
        int clipW = hs->width;
        int clipH = hs->height;
        if (hsFrame->clipW != -1 && hsFrame->clipH != -1) {
            int cx1 = clipX > hsFrame->clipX ? clipX : hsFrame->clipX;
            int cy1 = clipY > hsFrame->clipY ? clipY : hsFrame->clipY;
            int cx2 = (clipX + clipW) < (hsFrame->clipX + hsFrame->clipW) ? (clipX + clipW) : (hsFrame->clipX + hsFrame->clipW);
            int cy2 = (clipY + clipH) < (hsFrame->clipY + hsFrame->clipH) ? (clipY + clipH) : (hsFrame->clipY + hsFrame->clipH);
            clipX = cx1; clipY = cy1;
            clipW = cx2 > cx1 ? cx2 - cx1 : 0;
            clipH = cy2 > cy1 ? cy2 - cy1 : 0;
        }
        Frames[id].clipX = clipX;
        Frames[id].clipY = clipY;
        Frames[id].clipW = clipW;
        Frames[id].clipH = clipH;

        currentX += Frames[id].width + Frames[id].marginRight + hs->spacing;
    }
}

HStack* createHStack(int x, int y, int w, int h, int spacing) {
    HStack *hs = (HStack *)malloc(sizeof(HStack));
    hs->x = x;
    hs->y = y;
    hs->width = w;
    hs->height = h;
    hs->scrollX = 0;
    hs->spacing = spacing;
    hs->totalWidth = 0;
    hs->maxChildren = 32;
    hs->childFrameIds = (int *)malloc(sizeof(int) * hs->maxChildren);
    hs->childCount = 0;
    hs->isDragging = false;
    hs->frameId = requestFrame(w, h, x, y, hs, hstackPreRender, NULL, NULL, hstackOnTouch);
    Frames[hs->frameId].scrollType = 2;
    return hs;
}

void addFrameToHStack(HStack *hs, int id) {
    if (hs->childCount < hs->maxChildren) {
        hs->childFrameIds[hs->childCount++] = id;
        Frames[id].parentId = hs->frameId;
        renderFlag = true;
    }
}
