#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "font/font.h"
#include "TensorUI/Label/Label.h"
#include "hal/screen/screen.h"

int main()
{

    Font font_20 = loadFont("./fonts/LiberationSans-Regular20.bfont");
    Font font_15 = loadFont("./fonts/LiberationSans-Regular15.bfont");
    if (font_20.char_height == 0 || font_15.char_height == 0)
    { // Check if font loading failed
        fprintf(stderr, "Failed to load font. Exiting.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    init_screen();
    Label label1 = createLabel(100, 100, 40, 40, "Hello World!", COLOR_GREEN, COLOR_BLUE/*COLOR_TRANSPARENT*/, font_15);
    Label label3 = createLabel(120, 120, 40, 40, "Top Layer", COLOR_YELLOW, COLOR_TRANSPARENT, font_20);
    while (true)
    {
        fillScreen(COLOR_BLACK);
        if (!updateScreen())
            return 1;
    }

    return 0;
}
