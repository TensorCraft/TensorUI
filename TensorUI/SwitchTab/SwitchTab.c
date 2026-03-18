#include "SwitchTab.h"
#include "../../hal/mem/mem.h"
#include "../../hal/str/str.h"
#include "../../hal/screen/screen.h"
#include "../Font/font.h"

static Color switchTabGetPixel(void *self, int x, int y) {
    SwitchTab *st = (SwitchTab *)self;
    int tabWidth = st->width / st->numTabs;
    int currentTab = x / tabWidth;

    if (st->textBuffer && st->textBuffer[y * st->width + x]) {
        return COLOR_WHITE;
    }

    if (y > st->height - 4) { // Active indicator bar at bottom
        if (currentTab == st->activeTab) return (Color){0, 122, 255, false};
    }
    
    return (Color){40, 40, 40, false}; // Dark gray background
}

static void switchTabPreRender(void *self) {
    SwitchTab *st = (SwitchTab *)self;
    if (st->textBuffer) hal_free(st->textBuffer);
    st->textBuffer = (bool *)hal_malloc(st->width * st->height * sizeof(bool));
    hal_memset(st->textBuffer, 0, st->width * st->height * sizeof(bool));

    int tabWidth = st->width / st->numTabs;
    int textHeight = getTextHeight(st->font);

    for (int i = 0; i < st->numTabs; i++) {
        int textWidth = getTextWidth(st->tabNames[i], st->font);
        bool *bitmap = getTextBitmap(st->font, st->tabNames[i]);
        
        int startX = i * tabWidth + (tabWidth - textWidth) / 2;
        int startY = (st->height - textHeight) / 2;

        for (int row = 0; row < textHeight; row++) {
            for (int col = 0; col < textWidth; col++) {
                int targetX = startX + col;
                int targetY = startY + row;
                if (targetX < st->width && targetY < st->height) {
                    st->textBuffer[targetY * st->width + targetX] = bitmap[row * textWidth + col];
                }
            }
        }
    }
}

static void switchTabOnClick(void *self) {
    SwitchTab *st = (SwitchTab *)self;
    int mx, my;
    getTouchState(&mx, &my);
    
    int localX = mx - st->x;
    int tabWidth = st->width / st->numTabs;
    int clickedTab = localX / tabWidth;
    
    if (clickedTab >= 0 && clickedTab < st->numTabs) {
        st->activeTab = clickedTab;
        if (st->onTabChange) st->onTabChange(clickedTab);
    }
}

SwitchTab* createSwitchTab(int x, int y, int w, int h, int numTabs, char **names, Font font, void (*onTabChange)(int index)) {
    SwitchTab *st = (SwitchTab *)hal_malloc(sizeof(SwitchTab));
    st->x = x;
    st->y = y;
    st->width = w;
    st->height = h;
    st->numTabs = numTabs;
    st->activeTab = 0;
    st->tabNames = names;
    st->font = font;
    st->textBuffer = NULL;
    st->onTabChange = onTabChange;

    switchTabPreRender(st);
    st->frameId = requestFrame(w, h, x, y, st, switchTabPreRender, switchTabGetPixel, switchTabOnClick, NULL);
    if (st->frameId == -1) {
        if (st->textBuffer) hal_free(st->textBuffer);
        hal_free(st);
        return NULL;
    }
    return st;
}
