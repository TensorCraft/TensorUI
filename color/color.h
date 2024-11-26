#ifndef COLOR_H
#define COLOR_H
#include "stdbool.h"

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    bool transparent;
} Color;

extern const Color COLOR_TRANSPARENT;
extern const Color COLOR_RED;
extern const Color COLOR_GREEN;
extern const Color COLOR_BLUE;
extern const Color COLOR_WHITE;
extern const Color COLOR_BLACK;
extern const Color COLOR_YELLOW;
extern const Color COLOR_CYAN;
extern const Color COLOR_MAGENTA;
extern const Color COLOR_ORANGE;
extern const Color COLOR_PURPLE;
extern const Color COLOR_BROWN;
extern const Color COLOR_GRAY;
extern const Color COLOR_LIGHT_GRAY;
extern const Color COLOR_DARK_GRAY;
extern const Color COLOR_PINK;

#endif