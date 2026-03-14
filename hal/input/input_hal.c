#include "input_hal.h"
#include <SDL2/SDL.h>

bool hal_input_init() {
    return true;
}

PointerState hal_input_get_state() {
    PointerState state;
    int x, y;
    Uint32 buttons = SDL_GetMouseState(&x, &y);
    state.x = x;
    state.y = y;
    state.pressed = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    return state;
}

bool hal_input_poll(TouchEvent* outEvent) {
    // Platform specific polling logic would go here
    return false;
}

void getTouchState(int *x, int *y) {
    SDL_GetMouseState(x, y);
}
