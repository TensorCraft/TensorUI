#ifndef SCREEN_H
#define SCREEN_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "../../color/color.h"
#include "../../config.h"

// Global SDL variables
extern SDL_Window *window;
extern SDL_Renderer *renderer;

// Function declarations
bool init_screen();
void Pixel(int x, int y, Color color);
void fillScreen(Color color);
void fillRect(int x, int y, int w, int h, Color color);
bool updateScreen();
void close_screen();
bool init_touch();
void getTouchState(int *x, int *y);

// Function prototypes
int requestFrame(int width, int height, int x, int y, void *object, void (*preRender)(void *self), Color (*getPixel)(void *self, int x, int y), void (*onClick)(), void (*onTouch)());

void destroyFrame(int id);

typedef struct
{
    int width, height, x, y;
    void (*preRender)(void* self);
    Color (*getPixel)(void *, int, int);
    void (*onClick)();
    void (*onTouch)();
    bool enabled;
    bool needUpdate;
    void *object;
} Frame;

#endif // SCREEN_H
