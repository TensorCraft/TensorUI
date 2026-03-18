#ifndef FONT_H
#define FONT_H

#include "../../hal/screen/screen.h"
#include "../../config.h"

// Define a font structure to hold character widths and heights, and pixel data for each character
typedef struct {
    char char_widths[128]; // Width for each character (ASCII 0-127)
    char char_height;      // Common height for all characters
    char font_size;
    const char *char_data[128]; // Pixel data for each character as arrays of 1s and 0s
} Font;

// Functions to load and draw fonts
Font loadFont(const char *path);
void drawCharacter(Font font, int x, int y, char c, Color color);
void drawText(Font font, int x, int y, const char *str, Color color);
bool* getFontData(Font font, char c);
bool* getTextBitmap(Font font, const char *str);
int getTextWidth(const char *text, Font font);
int getTextHeight(Font font);
int getCharacterWidth(Font font, char c);
int getCharacterHeight(Font font);

#endif // FONT_H
