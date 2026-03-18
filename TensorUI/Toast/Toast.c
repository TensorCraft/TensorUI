#include "Toast.h"
#include "../../hal/mem/mem.h"
#include "../../hal/str/str.h"
#include "../../hal/screen/screen.h"
#include "../../hal/time/time.h"

#define MAX_TOASTS 10

typedef struct {
    char queue[MAX_TOASTS][64];
    int durations[MAX_TOASTS];
    int head;
    int tail;
    bool isShowing;
    long long showStartTime;
    Font font;
    bool *textBitmap;
    int textWidth;
} ToastManagerData;

static ToastManagerData *tm = NULL;
extern bool renderFlag;
extern Frame Frames[];
static int toastFrameId = -1;

static Color toastGetPixel(void *self, int x, int y) {
    if(!tm->isShowing) return COLOR_TRANSPARENT;
    
    // Draw rounded rect background
    int w = Frames[toastFrameId].width;
    int h = Frames[toastFrameId].height;
    int r = 10;
    
    // Simple rounded rect logic
    if (x < r && y < r && (x-r)*(x-r) + (y-r)*(y-r) > r*r) return COLOR_TRANSPARENT;
    if (x > w-r - 1 && y < r && (x-(w-r-1))*(x-(w-r-1)) + (y-r)*(y-r) > r*r) return COLOR_TRANSPARENT;
    if (x < r && y > h-r - 1 && (x-r)*(x-r) + (y-(h-r-1))*(y-(h-r-1)) > r*r) return COLOR_TRANSPARENT;
    if (x > w-r - 1 && y > h-r - 1 && (x-(w-r-1))*(x-(w-r-1)) + (y-(h-r-1))*(y-(h-r-1)) > r*r) return COLOR_TRANSPARENT;

    int textX = (w - tm->textWidth) / 2;
    int textY = (h - tm->font.char_height) / 2;

    if (tm->textBitmap && x >= textX && x < textX + tm->textWidth && y >= textY && y < textY + tm->font.char_height) {
        if (tm->textBitmap[(y - textY) * tm->textWidth + (x - textX)]) {
            return COLOR_WHITE;
        }
    }

    return (Color){30, 30, 30, false}; // Very dark gray toast bg
}

static void toastPreRender(void *self) {
    if(tm->isShowing) {
        long long now = current_timestamp_ms();
        if (now - tm->showStartTime > tm->durations[tm->head]) {
            // Toast expired
            tm->isShowing = false;
            if(tm->textBitmap) {
                hal_free(tm->textBitmap);
                tm->textBitmap = NULL;
            }
            tm->head = (tm->head + 1) % MAX_TOASTS;
            // Keep enabled=true so preRender continues to run for the next message
            renderFlag = true;
        }
    }
    
    if(!tm->isShowing && tm->head != tm->tail) {
        // Show next toast
        tm->isShowing = true;
        tm->showStartTime = current_timestamp_ms();
        tm->textWidth = getTextWidth(tm->queue[tm->head], tm->font);
        tm->textBitmap = getTextBitmap(tm->font, tm->queue[tm->head]);
        
        int w = tm->textWidth + 40;
        int h = tm->font.char_height + 20;
        Frames[toastFrameId].width = w;
        Frames[toastFrameId].height = h;
        Frames[toastFrameId].fx = (float)((SCREEN_WIDTH - w) / 2);
        Frames[toastFrameId].fy = (float)(SCREEN_HEIGHT - h - 30); // Android-style bottom fixed position
        Frames[toastFrameId].x = (int)Frames[toastFrameId].fx;
        Frames[toastFrameId].y = (int)Frames[toastFrameId].fy;
        Frames[toastFrameId].clipW = -1; // Unclipped global layer
        Frames[toastFrameId].clipH = -1;
        Frames[toastFrameId].enabled = true;

        
        renderFlag = true;
    }
}

void initToastManager(Font font) {
    if(tm) return;
    tm = (ToastManagerData *)hal_malloc(sizeof(ToastManagerData));
    tm->head = 0;
    tm->tail = 0;
    tm->isShowing = false;
    tm->font = font;
    tm->textBitmap = NULL;
    
    // We register a permanent frame, always enabled to monitor the queue.
    toastFrameId = requestFrame(10, 10, 0, 0, NULL, toastPreRender, toastGetPixel, NULL, NULL);
    Frames[toastFrameId].enabled = true;
    configureFrameAsSystemSurface(toastFrameId, true);
    Frames[toastFrameId].continuousRender = true;
}

void showToast(const char *message, int duration_ms) {
    if(tm) {
        int next = (tm->tail + 1) % MAX_TOASTS;
        if(next != tm->head) {
            hal_strncpy(tm->queue[tm->tail], message, 63);
            tm->queue[tm->tail][63] = '\0';
            tm->durations[tm->tail] = duration_ms;
            tm->tail = next;
        }
    }
}
