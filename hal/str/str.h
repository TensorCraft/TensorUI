#ifndef HAL_STR_H
#define HAL_STR_H

#include <stddef.h>

void *hal_memcpy(void *dest, const void *src, size_t n);
void *hal_memset(void *s, int c, size_t n);
size_t hal_strlen(const char *s);
char *hal_strncpy(char *dest, const char *src, size_t n);
int hal_strcmp(const char *s1, const char *s2);
char *hal_strdup(const char *s);
char *hal_strtok(char *str, const char *delim);

#endif
