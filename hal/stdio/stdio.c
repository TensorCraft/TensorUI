#include "stdio.h"
#include <stdio.h>
#include <stdarg.h>

int hal_snprintf(char *str, unsigned long size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(str, size, format, args);
    va_end(args);
    return ret;
}

int hal_puts(const char *s) {
    return puts(s);
}

int hal_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    return ret;
}
