#include "screen.h"
#include "../../color/color.h"

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
Frame Frames[MAX_FRAMES];
int frame_count = 0;
bool renderFlag = true;

#ifdef ROUND_SCREEN
bool roundMask[SCREEN_WIDTH][SCREEN_HEIGHT];
#endif

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

void getTouchState(int *x, int *y)
{
    SDL_GetMouseState(x, y);
}

bool init_screen()
{
    frame_count = 0;
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
    if(color.transparent)
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
    for (int i = 0; i < frame_count; i++)
        if (Frames[i].needUpdate && Frames[i].preRender != NULL)
        {
            Frames[i].needUpdate = false;
            Frames[i].preRender(Frames[i].object);
            renderFlag = true;
        }
    if (renderFlag)
    {
        for (int i = 0; i < frame_count; i++)
            if (Frames[i].enabled)
            {
                for (int x = 0; x < Frames[i].width; x++)
                    for (int y = 0; y < Frames[i].height; y++)
                        if (Frames[i].getPixel != NULL)
                            Pixel(x + Frames[i].x, y + Frames[i].y, Frames[i].getPixel(Frames[i].object, x, y));
            }
    }
#ifdef ROUND_SCREEN
    if (renderFlag)
    {
        for (int i = 0; i < SCREEN_WIDTH; i++)
        {
            for (int j = 0; j < SCREEN_HEIGHT; j++)
            {
                if (roundMask[i][j])
                {
                    Pixel(i, j, COLOR_DARK_GRAY);
                }
            }
        }
    }
#endif
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            close_screen();
            return false;
        }
    }
    if (!renderFlag)
        return true;
    renderFlag = false;
    SDL_RenderPresent(renderer);
    return true;
}

void close_screen()
{
    for (int i = 0; i < frame_count; i++)
        Frames[i] = (Frame){0};
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

int requestFrame(int width, int height, int x, int y, void* object, void (*preRender)(void *self), Color (*getPixel)(void *self, int x, int y), void (*onClick)(), void (*onTouch)())
{
    if (frame_count == MAX_FRAMES)
    {
        return -1;
    }

    Frame *frame = &Frames[frame_count];

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

    return frame_count++;
}

void destroyFrame(int id)
{
    if (id < 0 || id >= MAX_FRAMES)
        return;
    Frames[id].enabled = false;
}