#include "Tween.h"
#include "../../hal/time/time.h"

#define MAX_TWEENS 64

static Tween tweens[MAX_TWEENS];

void initTweenEngine() {
    for (int i = 0; i < MAX_TWEENS; i++) tweens[i].active = false;
}

static float applyEasing(float t, EaseType ease) {
    switch (ease) {
        case EASE_IN_QUAD: return t * t;
        case EASE_OUT_QUAD: return t * (2 - t);
        case EASE_IN_OUT_QUAD: return t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t;
        default: return t;
    }
}

Tween* createTween(float *val, float end, int duration, EaseType ease) {
    for (int i = 0; i < MAX_TWEENS; i++) {
        if (!tweens[i].active) {
            tweens[i].value = val;
            tweens[i].start = *val;
            tweens[i].end = end;
            tweens[i].startTime = current_timestamp_ms();
            tweens[i].duration = duration;
            tweens[i].ease = ease;
            tweens[i].active = true;
            tweens[i].onComplete = NULL;
            return &tweens[i];
        }
    }
    return NULL;
}

void updateTweens() {
    long long now = current_timestamp_ms();
    for (int i = 0; i < MAX_TWEENS; i++) {
        if (tweens[i].active) {
            float elapsed = (float)(now - tweens[i].startTime);
            float progress = elapsed / tweens[i].duration;
            
            if (progress >= 1.0f) {
                *tweens[i].value = tweens[i].end;
                tweens[i].active = false;
                if (tweens[i].onComplete) tweens[i].onComplete(tweens[i].arg);
                renderFlag = true;
            } else {
                float easedProgress = applyEasing(progress, tweens[i].ease);
                *tweens[i].value = tweens[i].start + (tweens[i].end - tweens[i].start) * easedProgress;
                renderFlag = true;
            }
        }
    }
}
