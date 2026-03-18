#ifndef TIME_H
#define TIME_H

#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

long long current_timestamp_ms();
void hal_sleep_ms(unsigned int ms);

#endif
