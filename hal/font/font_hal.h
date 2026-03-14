#ifndef FONT_HAL_H
#define FONT_HAL_H

#include <stddef.h>

/**
 * Abstraction for font data access to support different platforms
 * (e.g., POSIX files on PC, Flash memory on MCU)
 */

void* hal_font_open(const char* path);
size_t hal_font_read(void* handle, void* buffer, size_t size);
void hal_font_close(void* handle);

#endif
