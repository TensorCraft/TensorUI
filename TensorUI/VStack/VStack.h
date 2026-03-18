#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include "../Color/color.h"

typedef struct {
    int x, y, width, height;
    float scrollY;
    int totalHeight;

    int *childFrameIds;
    int childCount;
    int maxChildren;
    bool isDragging;
    int lastMouseY;
    int frameId;
    int spacing;
    bool showScrollbar;
    Color scrollbarColor;
    long lastScrollTime;
} VStack;

VStack* createVStack(int x, int y, int w, int h, int spacing);
void addFrameToVStack(VStack *vs, int frameId);
void setVStackContentInsets(VStack *vs, int top, int right, int bottom, int left);
void setVStackScrollbar(VStack *vs, bool visible);
void setVStackScrollbarColor(VStack *vs, Color c);
void cancelVStackInteraction(VStack *vs);
void vstackOnDestroy(void *self);

typedef struct {
    int x, y, width, height;
    float scrollX;
    int totalWidth;

    int *childFrameIds;
    int childCount;
    int maxChildren;
    bool isDragging;
    int lastMouseX;
    int frameId;
    int spacing;
    bool showScrollbar;
    Color scrollbarColor;
    long lastScrollTime;
    bool paging;
} HStack;


HStack* createHStack(int x, int y, int w, int h, int spacing);
void addFrameToHStack(HStack *hs, int frameId);
void setHStackContentInsets(HStack *hs, int top, int right, int bottom, int left);
void setHStackScrollbar(HStack *hs, bool visible);
void setHStackScrollbarColor(HStack *hs, Color c);
void setHStackPaging(HStack *hs, bool paging);
void cancelHStackInteraction(HStack *hs);
void hstackOnDestroy(void *self);



#endif
