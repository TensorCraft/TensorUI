#include "str.h"
#include <string.h>
#include <stdlib.h>

void *hal_memcpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
}

void *hal_memset(void *s, int c, size_t n) {
    return memset(s, c, n);
}

size_t hal_strlen(const char *s) {
    return strlen(s);
}

char *hal_strncpy(char *dest, const char *src, size_t n) {
    return strncpy(dest, src, n);
}

int hal_strcmp(const char *s1, const char *s2) {
    return strcmp(s1, s2);
}

char *hal_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len + 1);
    return out;
}

char *hal_strtok(char *str, const char *delim) {
    return strtok(str, delim);
}
