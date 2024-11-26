#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "config.h"

// Define an empty font in case of load failure
const Font EMPTY_FONT = {.char_height = 0};

// Function to load the font data from a binary font file
Font loadFont(const char *path)
{
    Font font = EMPTY_FONT; // Initialize to empty font in case of failure
#ifdef DEBUG
    FILE *file = fopen(path, "rb");
    puts("Loading font at:");
    puts(path);
#endif
    if (!file)
    {
        fprintf(stderr, "Error: Font file %s not found.\n", path);
        return font; // Return empty font on error
    }

    // Read widths and height from file
    fread(font.char_widths, sizeof(unsigned char), 128, file);
    fread(&font.char_height, sizeof(unsigned char), 1, file);
    fread(&font.font_size, sizeof(unsigned char), 1, file);
#ifdef DEBUG
    printf("font size: %d\n", font.font_size);
#endif
    // Allocate and read pixel data for each character
    for (int i = 0; i < 128; i++)
    {
        int width = font.char_widths[i];
        int num_pixels = width * font.char_height;
        font.char_data[i] = malloc(num_pixels);
        fread((char *)font.char_data[i], sizeof(char), num_pixels, file);
    }

    fclose(file);
    return font;
}

// Function to get font data for a character as a boolean array
bool* getFontData(Font font, char c)
{
    int width = font.char_widths[(char)c];
    int height = font.char_height;
    const char *pixels = font.char_data[(char)c];

    // Allocate memory for the boolean array
    bool *boolArray = (bool *)malloc(width * height * sizeof(bool));

    // Fill the boolean array based on pixel data
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            // Set the value in the boolean array
            boolArray[i * width + j] = (pixels[i * width + j] != 0);
        }
    }

    return boolArray;
}

// Function to draw a character using pixel data
void drawCharacter(Font font, int x, int y, char c, Color color)
{
    if (c < 0 || c > 127)
        return; // Only ASCII characters

    int width = font.char_widths[(char)c];
    int height = font.char_height;
    const char *pixels = font.char_data[(char)c];

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            if (pixels[i * width + j] != 0)
            {
                Pixel(j + x, i + y, color);
            }
        }
    }
}

bool* getTextBitmap(Font font, const char *str)
{
    int textWidth = getTextWidth(str, font);
    int textHeight = getTextHeight(font);
    bool *boolArray = (bool *)malloc(textWidth * textHeight * sizeof(bool));
    int offset = 0;
    while (*str != '\0')
    {
        int width = font.char_widths[(char)*str];
        int height = font.char_height;
        const char *pixels = font.char_data[(char)*str];

        // Fill the boolean array based on pixel data
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                // Set the value in the boolean array
                boolArray[i * textWidth + j + offset] = (pixels[i * width + j] != 0);
            }
        }

        offset += width;
        str++;
    }
    return boolArray;
}

void drawText(Font font, int x, int y, const char *str, Color color)
{
    while (*str != '\0')
    {
        int width = font.char_widths[(char)*str];
        int height = font.char_height;
        if (x + width > SCREEN_WIDTH)
        {
            x = 0;
            y += height;
        }
        drawCharacter(font, x, y, *str, color);
        x += width;
        str++;
    }
}

// Get the width of a string based on character widths
int getTextWidth(const char *text, Font font)
{
    int width = 0;
    for (const char *p = text; *p; p++)
    {
        width += font.char_widths[(int)*p];
    }
    return width;
}

// Get the height of the text based on common character height
int getTextHeight(Font font)
{
    return font.char_height;
}

// Get width of a single character
int getCharacterWidth(Font font, char c)
{
    return font.char_widths[(int)c];
}

// Get height of a single character (same as font height)
int getCharacterHeight(Font font)
{
    return font.char_height;
}
