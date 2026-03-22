#ifndef SCREEN_BACKEND_H
#define SCREEN_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SCREEN_PLATFORM_EVENT_NONE = 0,
    SCREEN_PLATFORM_EVENT_QUIT,
    SCREEN_PLATFORM_EVENT_POINTER_DOWN,
    SCREEN_PLATFORM_EVENT_POINTER_MOVE,
    SCREEN_PLATFORM_EVENT_POINTER_UP,
    SCREEN_PLATFORM_EVENT_WINDOW_INACTIVE,
} ScreenPlatformEventType;

typedef struct {
    ScreenPlatformEventType type;
    int x;
    int y;
    uint32_t timestamp_ms;
} ScreenPlatformEvent;

bool screen_backend_init(const char *title, int width, int height);
void screen_backend_shutdown(void);
bool screen_backend_poll_event(ScreenPlatformEvent *event);
void screen_backend_set_pointer_capture(bool enabled);
bool screen_backend_get_pointer_state(int *x, int *y, bool *pressed);
uint32_t screen_backend_ticks(void);
void screen_backend_present(int x, int y, int width, int height,
                            const uint32_t *pixels, int pitch_pixels);

#endif
