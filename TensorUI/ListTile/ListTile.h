#ifndef LISTTILE_H
#define LISTTILE_H

#include "../Color/color.h"
#include "../Font/font.h"
#include <stdbool.h>

typedef struct {
    int x, y, width, height;
    char *title;
    char *subtitle;
    Font titleFont;
    Font subtitleFont;
    Color titleColor;
    Color subtitleColor;
    void (*onClick)(void* arg);
    void* arg_user;
    void* arg; // Internal pointer
    int frameId;
} ListTile;


ListTile* createListTile(int x, int y, int w, char *title, char *subtitle, Font tFont, Font sFont, void (*onClick)(void*), void* arg);

#endif
