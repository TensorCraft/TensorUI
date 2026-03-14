#ifndef STACK_H
#define STACK_H

#include <stdbool.h>

typedef struct {
    int x, y, width, height;
    int scrollY;
    int totalHeight;
    int *childFrameIds;
    int childCount;
    int maxChildren;
    bool isDragging;
    int lastMouseY;
    int frameId;
    int spacing;
} VStack;

VStack* createVStack(int x, int y, int w, int h, int spacing);
void addFrameToVStack(VStack *vs, int frameId);

typedef struct {
    int x, y, width, height;
    int scrollX;
    int totalWidth;
    int *childFrameIds;
    int childCount;
    int maxChildren;
    bool isDragging;
    int lastMouseX;
    int frameId;
    int spacing;
} HStack;

HStack* createHStack(int x, int y, int w, int h, int spacing);
void addFrameToHStack(HStack *hs, int frameId);

#endif
