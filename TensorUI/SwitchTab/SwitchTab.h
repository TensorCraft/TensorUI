#ifndef SWITCHTAB_H
#define SWITCHTAB_H

#include "../Color/color.h"
#include "../Font/font.h"

typedef struct {
    int x, y, width, height;
    int activeTab;
    int numTabs;
    char **tabNames;
    Font font;
    bool *textBuffer;
    void (*onTabChange)(int index);
    int frameId;
} SwitchTab;

SwitchTab* createSwitchTab(int x, int y, int w, int h, int numTabs, char **names, Font font, void (*onTabChange)(int index));

#endif
