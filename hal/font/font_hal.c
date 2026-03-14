#include "font_hal.h"
#include <stdio.h>

void* hal_font_open(const char* path) {
    return (void*)fopen(path, "rb");
}

size_t hal_font_read(void* handle, void* buffer, size_t size) {
    if (!handle) return 0;
    return fread(buffer, 1, size, (FILE*)handle);
}

void hal_font_close(void* handle) {
    if (handle) {
        fclose((FILE*)handle);
    }
}
