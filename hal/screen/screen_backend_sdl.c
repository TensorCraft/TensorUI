#include "screen_backend.h"

#include <SDL2/SDL.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *screenTexture = NULL;
static int windowWidth = 0;
static int windowHeight = 0;

bool screen_backend_init(const char *title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow(title ? title : "TensorUI Emulator",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width,
                              height,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        window = NULL;
        SDL_Quit();
        return false;
    }

    screenTexture = SDL_CreateTexture(renderer,
                                      SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      width,
                                      height);
    if (!screenTexture) {
        printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        renderer = NULL;
        window = NULL;
        SDL_Quit();
        return false;
    }

    windowWidth = width;
    windowHeight = height;
    return true;
}

void screen_backend_shutdown(void) {
    if (screenTexture) {
        SDL_DestroyTexture(screenTexture);
        screenTexture = NULL;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    windowWidth = 0;
    windowHeight = 0;
    SDL_Quit();
}

bool screen_backend_poll_event(ScreenPlatformEvent *event) {
    SDL_Event rawEvent;

    if (event) {
        *event = (ScreenPlatformEvent){0};
    }

    if (!SDL_PollEvent(&rawEvent)) {
        return false;
    }

    if (!event) {
        return true;
    }

    switch (rawEvent.type) {
        case SDL_QUIT:
            event->type = SCREEN_PLATFORM_EVENT_QUIT;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (rawEvent.button.button == SDL_BUTTON_LEFT) {
                event->type = SCREEN_PLATFORM_EVENT_POINTER_DOWN;
                event->x = rawEvent.button.x;
                event->y = rawEvent.button.y;
                event->timestamp_ms = rawEvent.button.timestamp;
            }
            break;
        case SDL_MOUSEMOTION:
            event->type = SCREEN_PLATFORM_EVENT_POINTER_MOVE;
            event->x = rawEvent.motion.x;
            event->y = rawEvent.motion.y;
            event->timestamp_ms = rawEvent.motion.timestamp;
            break;
        case SDL_MOUSEBUTTONUP:
            if (rawEvent.button.button == SDL_BUTTON_LEFT) {
                event->type = SCREEN_PLATFORM_EVENT_POINTER_UP;
                event->x = rawEvent.button.x;
                event->y = rawEvent.button.y;
                event->timestamp_ms = rawEvent.button.timestamp;
            }
            break;
        case SDL_WINDOWEVENT:
            if (rawEvent.window.event == SDL_WINDOWEVENT_FOCUS_LOST ||
                rawEvent.window.event == SDL_WINDOWEVENT_HIDDEN ||
                rawEvent.window.event == SDL_WINDOWEVENT_LEAVE) {
                event->type = SCREEN_PLATFORM_EVENT_WINDOW_INACTIVE;
                event->timestamp_ms = SDL_GetTicks();
            }
            break;
        default:
            break;
    }

    return true;
}

void screen_backend_set_pointer_capture(bool enabled) {
    SDL_CaptureMouse(enabled ? SDL_TRUE : SDL_FALSE);
}

bool screen_backend_get_pointer_state(int *x, int *y, bool *pressed) {
    if (!window) {
        if (x) *x = 0;
        if (y) *y = 0;
        if (pressed) *pressed = false;
        return false;
    }

    int globalX = 0;
    int globalY = 0;
    Uint32 buttons = SDL_GetGlobalMouseState(&globalX, &globalY);
    int windowX = 0;
    int windowY = 0;
    SDL_GetWindowPosition(window, &windowX, &windowY);

    if (x) *x = globalX - windowX;
    if (y) *y = globalY - windowY;
    if (pressed) *pressed = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    return true;
}

uint32_t screen_backend_ticks(void) {
    return (uint32_t)SDL_GetTicks();
}

void screen_backend_present(int x, int y, int width, int height,
                            const uint32_t *pixels, int pitch_pixels) {
    if (!renderer || !screenTexture || !pixels || width <= 0 || height <= 0) {
        return;
    }

    SDL_Rect dirtyRect = {
        .x = x,
        .y = y,
        .w = width,
        .h = height,
    };
    SDL_UpdateTexture(screenTexture,
                      &dirtyRect,
                      pixels + y * pitch_pixels + x,
                      pitch_pixels * (int)sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
