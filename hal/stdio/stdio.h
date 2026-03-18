#ifndef HAL_STDIO_H
#define HAL_STDIO_H

#include <stdarg.h>

int hal_snprintf(char *str, unsigned long size, const char *format, ...);
int hal_puts(const char *s);
int hal_printf(const char *format, ...);

#endif
