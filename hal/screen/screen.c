#include "screen.h"
#include "../../TensorUI/Color/color.h"
#include "../time/time.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
int frameCount = 0;
int focus = -1;
int dragFocus = -1;
bool isDragging = false;
int startTouchX = 0;
int startTouchY = 0;
bool renderFlag = true;
Frame Frames[MAX_FRAMES];
static int lastFocusId = -1;
static bool hasMovedSignificant = false;

// Function declarations
void Pixel(int x, int y, Color color);
static bool antiAliasingEnabled = false;

void setAntiAliasing(bool enabled) {
    antiAliasingEnabled = enabled;
    renderFlag = true;
}

#ifdef ROUND_SCREEN
bool roundMask[SCREEN_WIDTH][SCREEN_HEIGHT] = {0};
#endif

bool isFrameVisible(int id) {
    if (id < 0 || id >= MAX_FRAMES) return false;
    if (!Frames[id].enabled) return false;
    if (Frames[id].parentId == -1) return true;
    return isFrameVisible(Frames[id].parentId);
}

#ifdef ROUND_SCREEN
void precalculateRoundMask()
{
    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    for (int i = 0; i < SCREEN_WIDTH; i++)
    {
        for (int j = 0; j < SCREEN_HEIGHT; j++)
        {
            roundMask[i][j] = (i - centerX) * (i - centerX) + (j - centerY) * (j - centerY) > centerX * centerX;
        }
    }
}
#endif

bool init_touch()
{
    return true;
}

// Moved to input_hal.c
/*
void getTouchState(int *x, int *y)
{
    SDL_GetMouseState(x, y);
}
*/

bool init_screen(const char *title, int width, int height)
{
    frameCount = 0;
    for (int i = 0; i < MAX_FRAMES; i++)
        Frames[i].enabled = false;
#ifdef ROUND_SCREEN
    precalculateRoundMask();
#endif

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("TensorUI Emulator",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH,
                              SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (!window)
    {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_DestroyWindow(window);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    return true;
}

void fillScreen(Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderClear(renderer);
}

void Pixel(int x, int y, Color color)
{
    if (color.transparent)
        return;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderDrawPoint(renderer, x, y);
}

void Rect(int x, int y, int w, int h, Color color)
{
    for (int i = x; i <= w; i++)
    {
        Pixel(i, y, color);
        Pixel(i, y + h, color);
    }
    for (int j = y + 1; j < y + h; j++)
    {
        Pixel(x, j, color);
        Pixel(x + w, j, color);
    }
}

void Line(int x0, int y0, int x1, int y1, Color color)
{
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true)
    {
        Pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void fillRect(int x, int y, int w, int h, Color color)
{
    for (int i = x; i <= x + w; i++)
        for (int j = y; j <= y + h; j++)
            Pixel(i, j, color);
}

void Circle(int x, int y, int r, Color color)
{
    int dx = 0;
    int dy = r;
    int d = 1 - r;

    while (dx <= dy)
    {
        Pixel(x + dx, y + dy, color);
        Pixel(x - dx, y + dy, color);
        Pixel(x + dx, y - dy, color);
        Pixel(x - dx, y - dy, color);
        Pixel(x + dy, y + dx, color);
        Pixel(x - dy, y + dx, color);
        Pixel(x + dy, y - dx, color);
        Pixel(x - dy, y - dx, color);

        if (d < 0)
        {
            d += 2 * dx + 3;
        }
        else
        {
            d += 2 * (dx - dy) + 5;
            dy--;
        }
        dx++;
    }
}

void fillCircle(int x, int y, int r, Color color)
{
    int dx = 0;
    int dy = r;
    int d = 1 - r;

    while (dx <= dy)
    {
        for (int i = -dx; i <= dx; i++)
        {
            Pixel(x + i, y + dy, color);
            Pixel(x + i, y - dy, color);
        }
        for (int i = -dy; i <= dy; i++)
        {
            Pixel(x + i, y + dx, color);
            Pixel(x + i, y - dx, color);
        }

        if (d < 0)
        {
            d += 2 * dx + 3;
        }
        else
        {
            d += 2 * (dx - dy) + 5;
            dy--;
        }
        dx++;
    }
}

bool updateScreen()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            close_screen();
            return false;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                startTouchX = event.button.x;
                startTouchY = event.button.y;
                lastFocusId = -1;
                hasMovedSignificant = false;
                isDragging = false;
                
                // Find initial focus (just the topmost interactive item)
                for(int i = frameCount - 1; i >= 0; i--) {
                    if(isFrameVisible(i)) {
                        if (Frames[i].clipW != -1 && Frames[i].clipH != -1) {
                            if (event.button.x < Frames[i].clipX || event.button.x >= Frames[i].clipX + Frames[i].clipW ||
                                event.button.y < Frames[i].clipY || event.button.y >= Frames[i].clipY + Frames[i].clipH) {
                                continue;
                            }
                        }
                        if(event.button.x >= Frames[i].x && event.button.x <= Frames[i].x + Frames[i].width &&
                           event.button.y >= Frames[i].y && event.button.y <= Frames[i].y + Frames[i].height) {
                            
                            if (lastFocusId == -1 && (Frames[i].onClick != NULL)) {
                                lastFocusId = i;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (event.type == SDL_MOUSEMOTION)
        {
            if (event.motion.state & SDL_BUTTON_LMASK) {
                int dx = event.motion.x - startTouchX;
                int dy = event.motion.y - startTouchY;
                if (dx*dx + dy*dy >= 25 && !isDragging) {
                    isDragging = true;
                    hasMovedSignificant = true;
                    
                    // Determine scroll direction
                    int desiredType = (abs(dx) > abs(dy)) ? 2 : 1;
                    
                    // Search for a scroll container of this type under startTouch
                    for(int i = frameCount - 1; i >= 0; i--) {
                        if(isFrameVisible(i) && (Frames[i].scrollType == desiredType || Frames[i].scrollType == 3)) {
                            if(startTouchX >= Frames[i].x && startTouchX <= Frames[i].x + Frames[i].width &&
                               startTouchY >= Frames[i].y && startTouchY <= Frames[i].y + Frames[i].height) {
                               
                               dragFocus = i;
                               if(Frames[dragFocus].onTouch) Frames[dragFocus].onTouch(Frames[dragFocus].object, true);
                               break;
                            }
                        }
                    }
                }
            }
        }
        if (event.type == SDL_MOUSEBUTTONUP)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                // If it was a tap (not dragged much)
                if (!hasMovedSignificant && lastFocusId != -1) {
                    if (Frames[lastFocusId].onClick != NULL) {
                        Frames[lastFocusId].onClick(Frames[lastFocusId].object);
                    }
                }
                
                // End drag if any
                if (dragFocus != -1 && Frames[dragFocus].onTouch != NULL) {
                    Frames[dragFocus].onTouch(Frames[dragFocus].object, false);
                }
                
                lastFocusId = -1;
                dragFocus = -1;
                isDragging = false;
                renderFlag = true;
            }
        }
    }

    // Pulse all frame coordinate syncing/pre-rendering
    // Always call preRender even if hidden, to allow background logic (like Toast manager)
    for (int i = 0; i < frameCount; i++) {
        if (Frames[i].preRender != NULL) {
            Frames[i].preRender(Frames[i].object);
        }
    }

    // Reset needUpdate for compatibility
    for (int i = 0; i < frameCount; i++) {
        Frames[i].needUpdate = false;
    }

    if (!renderFlag)
        return true;

    renderFlag = false;
    fillScreen(COLOR_BLACK);
    
    // Draw frames
    for (int i = 0; i < frameCount; i++)
    {
        if (isFrameVisible(i) && Frames[i].getPixel != NULL)
        {
            Color lastColor = {0, 0, 0, true};
            if (Frames[i].bgcolor.transparent == false) {
                SDL_SetRenderDrawColor(renderer, Frames[i].bgcolor.r, Frames[i].bgcolor.g, Frames[i].bgcolor.b, 255);
                SDL_Rect fillRect = {Frames[i].x, Frames[i].y, Frames[i].width, Frames[i].height};
                SDL_RenderFillRect(renderer, &fillRect);
                lastColor = Frames[i].bgcolor; 
            }

            for (int x = 0; x < Frames[i].width; x++)
                for (int y = 0; y < Frames[i].height; y++) {
                    int screenX = x + Frames[i].x;
                    int screenY = y + Frames[i].y;

                    if (Frames[i].clipW != -1 && Frames[i].clipH != -1) {
                        if (screenX < Frames[i].clipX || screenX >= Frames[i].clipX + Frames[i].clipW ||
                            screenY < Frames[i].clipY || screenY >= Frames[i].clipY + Frames[i].clipH) {
                            continue;
                        }
                    }

                    Color c;
                    if (antiAliasingEnabled && x < Frames[i].width - 1 && y < Frames[i].height - 1) {
                        Color c1 = Frames[i].getPixel(Frames[i].object, x, y);
                        Color c2 = Frames[i].getPixel(Frames[i].object, x + 1, y);
                        Color c3 = Frames[i].getPixel(Frames[i].object, x, y + 1);
                        Color c4 = Frames[i].getPixel(Frames[i].object, x + 1, y + 1);
                        
                        if (c1.transparent) continue;
                        
                        c.r = (c1.r * 2 + c2.r + c3.r + c4.r) / 5;
                        c.g = (c1.g * 2 + c2.g + c3.g + c4.g) / 5;
                        c.b = (c1.b * 2 + c2.b + c3.b + c4.b) / 5;
                        c.transparent = false;
                    } else {
                        c = Frames[i].getPixel(Frames[i].object, x, y);
                    }

                    if (c.transparent) continue;
                    if (c.r != lastColor.r || c.g != lastColor.g || c.b != lastColor.b || lastColor.transparent) {
                        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
                        lastColor = c;
                    }
                    SDL_RenderDrawPoint(renderer, screenX, screenY);
                }
        }
    }

#ifdef ROUND_SCREEN
    SDL_SetRenderDrawColor(renderer, COLOR_DARK_GRAY.r, COLOR_DARK_GRAY.g, COLOR_DARK_GRAY.b, 255);
    for (int i = 0; i < SCREEN_WIDTH; i++)
    {
        for (int j = 0; j < SCREEN_HEIGHT; j++)
        {
            if (roundMask[i][j])
            {
                SDL_RenderDrawPoint(renderer, i, j);
            }
        }
    }
#endif

    SDL_RenderPresent(renderer);
    return true;
}

void close_screen()
{
    for (int i = 0; i < frameCount; i++)
        Frames[i] = (Frame){0};
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

int requestFrame(int width, int height, int x, int y, void *object, 
                 void (*preRender)(void *self), 
                 Color (*getPixel)(void *self, int x, int y), 
                 void (*onClick)(void *self), 
                 void (*onTouch)(void *self, bool isDown))
{
    if (frameCount == MAX_FRAMES)
    {
        return -1;
    }

    Frame *frame = &Frames[frameCount];

    frame->object = object;
    frame->width = width;
    frame->height = height;
    frame->x = x;
    frame->y = y;
    frame->preRender = preRender;
    frame->getPixel = getPixel;
    frame->onClick = onClick;
    frame->onTouch = onTouch;
    frame->enabled = true;
    frame->needUpdate = true;

    frame->scrollType = 0;
    frame->paddingTop = 0; frame->paddingBottom = 0; frame->paddingLeft = 0; frame->paddingRight = 0;
    frame->marginTop = 0; frame->marginBottom = 0; frame->marginLeft = 0; frame->marginRight = 0;
    frame->alignment = ALIGNMENT_LEFT | ALIGNMENT_TOP;

    frame->clipX = 0;
    frame->clipY = 0;
    frame->clipW = -1;
    frame->clipH = -1;
    frame->bgcolor = COLOR_TRANSPARENT;
    frame->parentId = -1;
 
    return frameCount++;
}

void destroyFrame(int id)
{
    if (id < 0 || id >= MAX_FRAMES)
        return;
    Frames[id].enabled = false;
}