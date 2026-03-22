#include "screen_backend.h"

/*
 * Embedded backend skeleton.
 *
 * Swap this file into the build in place of screen_backend_sdl.c when porting
 * TensorUI to a real display controller. The renderer in screen.c already
 * computes a single dirty rectangle; this backend's job is to translate that
 * rect into the target panel's windowed flush API.
 *
 * Typical implementation points:
 * - init panel bus, reset pin, and pixel format
 * - implement pointer/touch polling through the platform input stack
 * - push only the dirty window in screen_backend_present()
 * - optionally replace the single full-frame CPU buffer with a line buffer or
 *   DMA-backed transfer path if the target cannot keep a full framebuffer
 */

bool screen_backend_init(const char *title, int width, int height) {
    (void)title;
    (void)width;
    (void)height;
    return false;
}

void screen_backend_shutdown(void) {
}

bool screen_backend_poll_event(ScreenPlatformEvent *event) {
    if (event) {
        *event = (ScreenPlatformEvent){0};
    }
    return false;
}

void screen_backend_set_pointer_capture(bool enabled) {
    (void)enabled;
}

bool screen_backend_get_pointer_state(int *x, int *y, bool *pressed) {
    if (x) *x = 0;
    if (y) *y = 0;
    if (pressed) *pressed = false;
    return false;
}

uint32_t screen_backend_ticks(void) {
    return 0;
}

void screen_backend_present(int x, int y, int width, int height,
                            const uint32_t *pixels, int pitch_pixels) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)pixels;
    (void)pitch_pixels;
}
