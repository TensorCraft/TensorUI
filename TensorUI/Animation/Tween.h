#ifndef TWEEN_H
#define TWEEN_H

#include "../../hal/screen/screen.h"

typedef enum {
    EASE_LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD
} EaseType;

typedef struct {
    float *value;
    float start;
    float end;
    long long startTime;
    int duration;
    EaseType ease;
    void (*onComplete)(void *arg);
    void *arg;
    bool active;
} Tween;

void initTweenEngine();
Tween* createTween(float *val, float end, int duration, EaseType ease);
void updateTweens();

#endif
