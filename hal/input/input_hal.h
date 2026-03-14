#ifndef INPUT_HAL_H
#define INPUT_HAL_H

#include <stdbool.h>

typedef enum {
    TOUCH_START,
    TOUCH_MOVE,
    TOUCH_END,
    TOUCH_CANCEL
} TouchType;

typedef struct {
    TouchType type;
    int x;
    int y;
    int id; // For multi-touch support
} TouchEvent;

/**
 * HAL Abstraction for Input
 * On PC, this translates SDL events.
 * On Embedded, this reads I2C/SPI touch controllers (CST816, GT911, etc.)
 */

typedef struct {
    int x, y;
    bool pressed;
} PointerState;

bool hal_input_init();
bool hal_input_poll(TouchEvent* outEvent);
PointerState hal_input_get_state();

#endif
