#include "mem.h"
#include <stdlib.h>

void *hal_malloc(size_t size) {
    return malloc(size);
}

void hal_free(void *ptr) {
    free(ptr);
}

void *hal_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}
