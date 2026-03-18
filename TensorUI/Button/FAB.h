#ifndef FAB_H
#define FAB_H

#include "../Color/color.h"
#include <stdbool.h>

typedef struct {
    int x, y, radius;
    char *icon; // Could be a single char like '+'
    Color bgColor;
    Color iconColor;
    void (*onClick)(void* arg);
    void* arg;
    int frameId;
} FAB;

FAB* createFAB(int x, int y, char *icon, Color bgColor, Color iconColor, void (*onClick)(void*), void* arg);

#endif
