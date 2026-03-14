#ifndef SCREEN_H
#define SCREEN_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "../../TensorUI/Color/color.h"
#include "../../config.h"

// Alignment Constants
#define ALIGNMENT_TOP 0
#define ALIGNMENT_BOTTOM 1
#define ALIGNMENT_CENTER_VERTICAL 2
#define ALIGNMENT_LEFT 0
#define ALIGNMENT_RIGHT 4
#define ALIGNMENT_CENTER_HORIZONTAL 8

typedef struct
{
    int width, height, x, y;
    void (*preRender)(void* self);
    Color (*getPixel)(void *self, int x, int y); 
    void (*onClick)(void *self);
    void (*onTouch)(void *self, bool isDown);
    bool enabled;
    bool needUpdate;
    void *object;
    int scrollType; // 0=none, 1=vertical, 2=horizontal
    int paddingTop, paddingBottom, paddingLeft, paddingRight;
    int marginTop, marginBottom, marginLeft, marginRight;
    int alignment;
    int clipX, clipY, clipW, clipH;
    Color bgcolor;
    int parentId; // -1 if no parent
} Frame;

// Global variables
extern int frameCount;
extern int focus;
extern int dragFocus;
extern bool isDragging;
extern int startTouchX;
extern int startTouchY;
extern bool renderFlag;
extern SDL_Renderer *renderer;
extern Frame Frames[MAX_FRAMES];

// Function declarations
void setAntiAliasing(bool enabled);
bool init_screen(const char *title, int width, int height);
void Pixel(int x, int y, Color color);
void fillScreen(Color color);
void fillRect(int x, int y, int w, int h, Color color);
bool updateScreen();
void close_screen();
bool init_touch();
void getTouchState(int *x, int *y);

int requestFrame(int width, int height, int x, int y, void *object, 
                 void (*preRender)(void *self), 
                 Color (*getPixel)(void *self, int x, int y), 
                 void (*onClick)(void *self), 
                 void (*onTouch)(void *self, bool isDown));

void destroyFrame(int id);

#endif
