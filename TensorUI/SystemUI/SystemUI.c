#include "SystemUI.h"
#include "../../hal/mem/mem.h"
#include "../../hal/str/str.h"
#include "../../hal/screen/screen.h"
#include "../../hal/time/time.h"
#include "../Color/color.h"
#include "../Animation/Tween.h"
#include "../WindowManager.h"
#include <stdlib.h>

#define SYSTEMUI_MAX_NOTIFICATIONS 8
#define STATUS_BAR_HEIGHT 24
#define SHADE_TOP_ZONE_PX 18
#define SHADE_LOCK_DY_PX 10
#define SHADE_CLOSE_LOCK_DY_PX 8
#define SHADE_OPEN_THRESHOLD 72
#define SHADE_CLOSE_THRESHOLD 56
#define SHADE_CARD_MARGIN 10
#define SHADE_CARD_HEIGHT 46
#define SHADE_CLOSE_ZONE_PX 28
#define SYSTEMUI_MAX_ICONS 3

typedef struct {
    char title[64];
    char message[128];
    char displayTitle[64];
    char displayMessage[128];
    bool *titleBitmap;
    bool *messageBitmap;
    int titleW;
    int messageW;
    int titleH;
    int messageH;
    SystemNotificationClickHandler onClick;
    void *clickContext;
} SystemNotification;

typedef struct {
    const Color *pixels;
    int srcW;
    int srcH;
    int size;
    bool visible;
} StatusBarIcon;

typedef struct {
    bool initialized;
    int frameId;
    Font statusFont;
    Font bodyFont;
    char title[32];
    char clockText[16];
    bool clockPinned;
    bool *titleBitmap;
    bool *clockBitmap;
    int titleW;
    int clockW;
    StatusBarIcon icons[SYSTEMUI_MAX_ICONS];
    long long lastClockRefreshMs;
    SystemNotification notifications[SYSTEMUI_MAX_NOTIFICATIONS];
    int notificationCount;
    float shadeReveal;
    bool shadeDragging;
    bool shadeOpen;
    bool shadeClosing;
    int shadeStartY;
    int shadeStartReveal;
    int frozenWindowId;
} SystemUIState;

static SystemUIState ui = {0};

extern Frame Frames[];
extern bool renderFlag;

static void freeBitmap(bool **bitmap) {
    if (*bitmap) {
        hal_free(*bitmap);
        *bitmap = NULL;
    }
}

static int systemStatusBarHeight(void) {
#ifdef ROUND_SCREEN
    return 0;
#else
    return STATUS_BAR_HEIGHT;
#endif
}

static void freezeTopWindowIfNeeded(void) {
    if (ui.frozenWindowId != -1) return;
    int topId = getTopWindow();
    if (topId < 0 || topId >= MAX_FRAMES) return;
    Frames[topId].isPaused = true;
    if (Frames[topId].onPause) Frames[topId].onPause(Frames[topId].object);
    ui.frozenWindowId = topId;
}

static void restoreFrozenWindowIfNeeded(void) {
    if (ui.frozenWindowId < 0 || ui.frozenWindowId >= MAX_FRAMES) {
        ui.frozenWindowId = -1;
        return;
    }
    if (Frames[ui.frozenWindowId].enabled) {
        Frames[ui.frozenWindowId].isPaused = false;
        if (Frames[ui.frozenWindowId].onResume) Frames[ui.frozenWindowId].onResume(Frames[ui.frozenWindowId].object);
    }
    ui.frozenWindowId = -1;
}

static void clearNotificationAt(int index) {
    if (index < 0 || index >= ui.notificationCount) return;
    freeBitmap(&ui.notifications[index].titleBitmap);
    freeBitmap(&ui.notifications[index].messageBitmap);
    for (int i = index; i < ui.notificationCount - 1; i++) {
        ui.notifications[i] = ui.notifications[i + 1];
    }
    ui.notificationCount--;
    if (ui.notificationCount < 0) ui.notificationCount = 0;
    if (ui.notificationCount < SYSTEMUI_MAX_NOTIFICATIONS) {
        ui.notifications[ui.notificationCount] = (SystemNotification){0};
    }
}

static int getNotificationCardY(int index) {
    int topInset = systemStatusBarHeight() > 0 ? systemStatusBarHeight() + 10 : 12;
    return topInset + index * (SHADE_CARD_HEIGHT + 8);
}

static int getNotificationCardWidth(void) {
    return SCREEN_WIDTH - SHADE_CARD_MARGIN * 2;
}

static int findNotificationAtPoint(int px, int py) {
    if (!ui.shadeOpen) return -1;
    int cardW = getNotificationCardWidth();
    for (int i = 0; i < ui.notificationCount; i++) {
        int cardY = getNotificationCardY(i);
        if (px >= SHADE_CARD_MARGIN && px < SHADE_CARD_MARGIN + cardW &&
            py >= cardY && py < cardY + SHADE_CARD_HEIGHT &&
            py < (int)ui.shadeReveal) {
            return i;
        }
    }
    return -1;
}

static Color drawStatusIconPixel(const StatusBarIcon *icon, int x, int y, int originX, int originY) {
    if (!icon || !icon->visible || !icon->pixels || icon->srcW <= 0 || icon->srcH <= 0 || icon->size <= 0) {
        return COLOR_TRANSPARENT;
    }

    int lx = x - originX;
    int ly = y - originY;
    if (lx < 0 || ly < 0 || lx >= icon->size || ly >= icon->size) return COLOR_TRANSPARENT;
    int srcX = (lx * icon->srcW) / icon->size;
    int srcY = (ly * icon->srcH) / icon->size;
    if (srcX < 0 || srcY < 0 || srcX >= icon->srcW || srcY >= icon->srcH) return COLOR_TRANSPARENT;
    return icon->pixels[srcY * icon->srcW + srcX];
}

static void truncateWithEllipsis(const char *input, char *output, int outputCap, Font font, int maxWidth) {
    if (!input || !output || outputCap <= 0) return;
    output[0] = '\0';

    if (getTextWidth(input, font) <= maxWidth) {
        hal_strncpy(output, input, outputCap - 1);
        output[outputCap - 1] = '\0';
        return;
    }

    const char *ellipsis = "...";
    int ellipsisW = getTextWidth(ellipsis, font);
    int len = (int)hal_strlen(input);
    int outLen = 0;
    for (int i = 0; i < len && outLen < outputCap - 4; i++) {
        output[outLen++] = input[i];
        output[outLen] = '\0';
        if (getTextWidth(output, font) + ellipsisW > maxWidth) {
            outLen--;
            if (outLen < 0) outLen = 0;
            output[outLen] = '\0';
            break;
        }
    }

    hal_strncpy(output + outLen, ellipsis, outputCap - outLen - 1);
    output[outputCap - 1] = '\0';
}

static void rebuildStatusBitmaps(void) {
    freeBitmap(&ui.titleBitmap);
    freeBitmap(&ui.clockBitmap);
    ui.titleW = getTextWidth(ui.title, ui.statusFont);
    ui.clockW = getTextWidth(ui.clockText, ui.statusFont);
    ui.titleBitmap = getTextBitmap(ui.statusFont, ui.title);
    ui.clockBitmap = getTextBitmap(ui.statusFont, ui.clockText);
}

static void rebuildNotificationBitmap(SystemNotification *notification) {
    if (!notification) return;
    freeBitmap(&notification->titleBitmap);
    freeBitmap(&notification->messageBitmap);
    notification->titleW = getTextWidth(notification->displayTitle, ui.statusFont);
    notification->messageW = getTextWidth(notification->displayMessage, ui.bodyFont);
    notification->titleH = getTextHeight(ui.statusFont);
    notification->messageH = getTextHeight(ui.bodyFont);
    notification->titleBitmap = getTextBitmap(ui.statusFont, notification->displayTitle);
    notification->messageBitmap = getTextBitmap(ui.bodyFont, notification->displayMessage);
}

static void refreshClockIfNeeded(void) {
    if (ui.clockPinned) return;
    long long now = current_timestamp_ms();
    if (now - ui.lastClockRefreshMs < 1000) return;
    ui.lastClockRefreshMs = now;
    char nextClock[16];
    long long totalMinutes = (now / 60000LL) % (24LL * 60LL);
    int hours = (int)(totalMinutes / 60LL);
    int minutes = (int)(totalMinutes % 60LL);
    snprintf(nextClock, sizeof(nextClock), "%02d:%02d", hours, minutes);
    if (hal_strcmp(nextClock, ui.clockText) != 0) {
        hal_strncpy(ui.clockText, nextClock, sizeof(ui.clockText) - 1);
        ui.clockText[sizeof(ui.clockText) - 1] = '\0';
        rebuildStatusBitmaps();
        invalidateFrame(ui.frameId);
    }
}

static int notificationShadeHeight(void) {
    (void)ui.notificationCount;
    return SCREEN_HEIGHT;
}

static Color drawTextBitmap(bool *bitmap, int bitmapW, int bitmapH, int x, int y, int originX, int originY, Color color) {
    if (!bitmap) return COLOR_TRANSPARENT;
    int lx = x - originX;
    int ly = y - originY;
    if (lx < 0 || ly < 0 || lx >= bitmapW || ly >= bitmapH) return COLOR_TRANSPARENT;
    if (bitmap[ly * bitmapW + lx]) return color;
    return COLOR_TRANSPARENT;
}

static Color systemUIGetPixel(void *self, int x, int y) {
    (void)self;
    int statusBarHeight = systemStatusBarHeight();
    int reveal = (int)ui.shadeReveal;
    bool shadeVisible = reveal > statusBarHeight;

    if (!shadeVisible && statusBarHeight > 0 && y < statusBarHeight) {
        Color bg = (Color){18, 18, 22, false};
        int textY = (statusBarHeight - getTextHeight(ui.statusFont)) / 2;
        Color titlePixel = drawTextBitmap(ui.titleBitmap, ui.titleW, getTextHeight(ui.statusFont), x, y, 10, textY, COLOR_WHITE);
        if (!titlePixel.transparent) return titlePixel;

        int rightX = SCREEN_WIDTH - ui.clockW - 10;
        for (int i = SYSTEMUI_MAX_ICONS - 1; i >= 0; i--) {
            const StatusBarIcon *icon = &ui.icons[i];
            if (!icon->visible) continue;
            rightX -= icon->size;
            Color iconPixel = drawStatusIconPixel(icon, x, y, rightX, (statusBarHeight - icon->size) / 2);
            if (!iconPixel.transparent) return iconPixel;
            rightX -= 6;
        }

        Color clockPixel = drawTextBitmap(ui.clockBitmap, ui.clockW, getTextHeight(ui.statusFont), x, y, SCREEN_WIDTH - ui.clockW - 10, textY, M3_PRIMARY);
        if (!clockPixel.transparent) return clockPixel;
        return bg;
    }

    if (reveal <= statusBarHeight || y >= reveal) {
        return COLOR_TRANSPARENT;
    }

    Color shadeBg = (Color){18, 18, 22, false};
    if (statusBarHeight > 0 && y < statusBarHeight) {
        int textY = (statusBarHeight - getTextHeight(ui.statusFont)) / 2;
        Color titlePixel = drawTextBitmap(ui.titleBitmap, ui.titleW, getTextHeight(ui.statusFont), x, y, 10, textY, COLOR_WHITE);
        if (!titlePixel.transparent) return titlePixel;

        int rightX = SCREEN_WIDTH - ui.clockW - 10;
        for (int i = SYSTEMUI_MAX_ICONS - 1; i >= 0; i--) {
            const StatusBarIcon *icon = &ui.icons[i];
            if (!icon->visible) continue;
            rightX -= icon->size;
            Color iconPixel = drawStatusIconPixel(icon, x, y, rightX, (statusBarHeight - icon->size) / 2);
            if (!iconPixel.transparent) return iconPixel;
            rightX -= 6;
        }

        Color clockPixel = drawTextBitmap(ui.clockBitmap, ui.clockW, getTextHeight(ui.statusFont), x, y, SCREEN_WIDTH - ui.clockW - 10, textY, M3_PRIMARY);
        if (!clockPixel.transparent) return clockPixel;
        return shadeBg;
    }

    for (int i = 0; i < ui.notificationCount; i++) {
        SystemNotification *notification = &ui.notifications[i];
        int cardY = getNotificationCardY(i);
        int cardW = getNotificationCardWidth();
        bool insideCard = x >= SHADE_CARD_MARGIN && x < SHADE_CARD_MARGIN + cardW &&
                          y >= cardY && y < cardY + SHADE_CARD_HEIGHT &&
                          y < reveal;
        if (!insideCard) continue;

        int radius = 10;
        int lx = x - SHADE_CARD_MARGIN;
        int ly = y - cardY;
        bool inCorner = false;
        int cx = 0, cy = 0;
        if (lx < radius && ly < radius) { cx = radius; cy = radius; inCorner = true; }
        else if (lx >= cardW - radius && ly < radius) { cx = cardW - radius - 1; cy = radius; inCorner = true; }
        else if (lx < radius && ly >= SHADE_CARD_HEIGHT - radius) { cx = radius; cy = SHADE_CARD_HEIGHT - radius - 1; inCorner = true; }
        else if (lx >= cardW - radius && ly >= SHADE_CARD_HEIGHT - radius) { cx = cardW - radius - 1; cy = SHADE_CARD_HEIGHT - radius - 1; inCorner = true; }
        if (inCorner && ((lx - cx) * (lx - cx) + (ly - cy) * (ly - cy) > radius * radius)) {
            return COLOR_TRANSPARENT;
        }

        Color titlePixel = drawTextBitmap(notification->titleBitmap, notification->titleW, notification->titleH,
                                          x, y, SHADE_CARD_MARGIN + 10, cardY + 8, COLOR_WHITE);
        if (!titlePixel.transparent) return titlePixel;
        Color msgPixel = drawTextBitmap(notification->messageBitmap, notification->messageW, notification->messageH,
                                        x, y, SHADE_CARD_MARGIN + 10, cardY + 24, M3_OUTLINE);
        if (!msgPixel.transparent) return msgPixel;
        return (Color){40, 40, 44, false};
    }

    return shadeBg;
}

static void systemUIOnClick(void *self) {
    (void)self;
    if (!ui.shadeOpen || ui.shadeDragging) return;

    int px = 0;
    int py = 0;
    getTouchState(&px, &py);
    int index = findNotificationAtPoint(px, py);
    if (index == -1) return;

    SystemNotificationClickHandler onClick = ui.notifications[index].onClick;
    void *context = ui.notifications[index].clickContext;
    clearNotificationAt(index);
    closeNotificationShade();
    invalidateFrame(ui.frameId);

    if (onClick) {
        onClick(context);
    }
}

static void systemUIPreRender(void *self) {
    (void)self;
    refreshClockIfNeeded();

    bool wantsTracking = false;
    int statusBarHeight = systemStatusBarHeight();
    if (currentInputSession.active) {
        int dx = currentInputSession.currentX - currentInputSession.startX;
        int dy = currentInputSession.currentY - currentInputSession.startY;
        bool downwardIntent = dy > SHADE_LOCK_DY_PX && abs(dy) > abs(dx) + 2;
        bool upwardIntent = dy < -SHADE_CLOSE_LOCK_DY_PX && abs(dy) > abs(dx) + 2;
        int closeZoneTop = SCREEN_HEIGHT - SHADE_CLOSE_ZONE_PX;

        if (!ui.shadeDragging && !ui.shadeOpen &&
            currentInputSession.startY < SHADE_TOP_ZONE_PX && downwardIntent) {
            ui.shadeDragging = true;
            ui.shadeStartY = currentInputSession.startY;
            ui.shadeStartReveal = systemStatusBarHeight();
            freezeTopWindowIfNeeded();
        } else if (!ui.shadeDragging && ui.shadeOpen &&
                   currentInputSession.startY >= closeZoneTop &&
                   currentInputSession.startY <= SCREEN_HEIGHT &&
                   upwardIntent) {
            ui.shadeDragging = true;
            ui.shadeStartY = currentInputSession.startY;
            ui.shadeStartReveal = (int)ui.shadeReveal;
        }

        if (ui.shadeDragging) {
            int maxReveal = notificationShadeHeight();
            int reveal = ui.shadeStartReveal + dy;
            if (reveal < statusBarHeight) reveal = statusBarHeight;
            if (reveal > maxReveal) reveal = maxReveal;
            ui.shadeReveal = (float)reveal;
            ui.shadeOpen = reveal > statusBarHeight;
            ui.shadeClosing = false;
            wantsTracking = true;
            invalidateFrame(ui.frameId);
        }
    } else if (ui.shadeDragging) {
        ui.shadeDragging = false;
        int target = statusBarHeight;
        if (ui.shadeStartReveal <= statusBarHeight) {
            target = (ui.shadeReveal >= SHADE_OPEN_THRESHOLD) ? notificationShadeHeight() : statusBarHeight;
        } else {
            int collapsed = SCREEN_HEIGHT - (int)ui.shadeReveal;
            target = (collapsed >= SHADE_CLOSE_THRESHOLD) ? statusBarHeight : notificationShadeHeight();
        }
        ui.shadeOpen = target > statusBarHeight;
        ui.shadeClosing = target == statusBarHeight;
        createTween(&ui.shadeReveal, (float)target, 180, EASE_OUT_QUAD);
        if (ui.shadeOpen) {
            freezeTopWindowIfNeeded();
        }
        invalidateFrame(ui.frameId);
    } else if (ui.shadeClosing && ui.shadeReveal <= statusBarHeight + 0.5f) {
        ui.shadeReveal = (float)statusBarHeight;
        ui.shadeClosing = false;
        restoreFrozenWindowIfNeeded();
        invalidateFrame(ui.frameId);
    } else if (!ui.shadeOpen && !ui.shadeClosing && ui.frozenWindowId != -1) {
        restoreFrozenWindowIfNeeded();
        invalidateFrame(ui.frameId);
    }

    Frames[ui.frameId].onClick = (ui.shadeOpen && !ui.shadeDragging) ? systemUIOnClick : NULL;
    Frames[ui.frameId].continuousRender = wantsTracking || ui.shadeClosing;
}

void initSystemUI(Font statusFont, Font bodyFont) {
    if (ui.initialized) return;
    ui.initialized = true;
    ui.statusFont = statusFont;
    ui.bodyFont = bodyFont;
    ui.frozenWindowId = -1;
    hal_strncpy(ui.title, "TensorOS", sizeof(ui.title) - 1);
    ui.title[sizeof(ui.title) - 1] = '\0';
    ui.clockText[0] = '\0';
    ui.clockPinned = false;
    ui.frameId = requestFrame(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, NULL, systemUIPreRender, systemUIGetPixel, systemUIOnClick, NULL);
    configureFrameAsSystemSurface(ui.frameId, true);
    Frames[ui.frameId].enabled = true;
    Frames[ui.frameId].continuousRender = true;
    ui.shadeReveal = (float)systemStatusBarHeight();
    ui.shadeClosing = false;
    rebuildStatusBitmaps();
    refreshClockIfNeeded();
}

void updateSystemUI(void) {
    if (!ui.initialized) return;
    systemUIPreRender(NULL);
}

void setStatusBarTitle(const char *title) {
    if (!ui.initialized || !title) return;
    hal_strncpy(ui.title, title, sizeof(ui.title) - 1);
    ui.title[sizeof(ui.title) - 1] = '\0';
    rebuildStatusBitmaps();
    invalidateFrame(ui.frameId);
}

void setStatusBarClockText(const char *clockText) {
    if (!ui.initialized || !clockText) return;
    hal_strncpy(ui.clockText, clockText, sizeof(ui.clockText) - 1);
    ui.clockText[sizeof(ui.clockText) - 1] = '\0';
    ui.clockPinned = true;
    rebuildStatusBitmaps();
    invalidateFrame(ui.frameId);
}

void setStatusBarIcon(int slot, const Color *pixels, int srcW, int srcH, int size) {
    if (!ui.initialized) return;
    if (slot < 0 || slot >= SYSTEMUI_MAX_ICONS) return;
    StatusBarIcon *icon = &ui.icons[slot];
    icon->pixels = pixels;
    icon->srcW = srcW;
    icon->srcH = srcH;
    if (size < 8) size = 8;
    if (size > 12) size = 12;
    icon->size = size;
    icon->visible = pixels != NULL && srcW > 0 && srcH > 0;
    invalidateFrame(ui.frameId);
}

void clearStatusBarIcon(int slot) {
    if (!ui.initialized) return;
    if (slot < 0 || slot >= SYSTEMUI_MAX_ICONS) return;
    ui.icons[slot] = (StatusBarIcon){0};
    invalidateFrame(ui.frameId);
}

void pushSystemNotification(const char *title, const char *message) {
    pushSystemNotificationWithAction(title, message, NULL, NULL);
}

void pushSystemNotificationWithAction(const char *title, const char *message,
                                      SystemNotificationClickHandler onClick, void *context) {
    if (!ui.initialized) return;
    if (ui.notificationCount >= SYSTEMUI_MAX_NOTIFICATIONS) {
        clearNotificationAt(SYSTEMUI_MAX_NOTIFICATIONS - 1);
    } else {
        ui.notificationCount++;
    }

    for (int i = ui.notificationCount - 1; i > 0; i--) {
        ui.notifications[i] = ui.notifications[i - 1];
    }
    ui.notifications[0] = (SystemNotification){0};

    SystemNotification *notification = &ui.notifications[0];
    hal_strncpy(notification->title, title ? title : "Notification", sizeof(notification->title) - 1);
    notification->title[sizeof(notification->title) - 1] = '\0';
    hal_strncpy(notification->message, message ? message : "", sizeof(notification->message) - 1);
    notification->message[sizeof(notification->message) - 1] = '\0';
    notification->onClick = onClick;
    notification->clickContext = context;
    truncateWithEllipsis(notification->title, notification->displayTitle, sizeof(notification->displayTitle), ui.statusFont, SCREEN_WIDTH - 40);
    truncateWithEllipsis(notification->message, notification->displayMessage, sizeof(notification->displayMessage), ui.bodyFont, SCREEN_WIDTH - 40);
    rebuildNotificationBitmap(notification);
    invalidateFrame(ui.frameId);
}

void clearSystemNotifications(void) {
    for (int i = 0; i < ui.notificationCount; i++) {
        freeBitmap(&ui.notifications[i].titleBitmap);
        freeBitmap(&ui.notifications[i].messageBitmap);
    }
    ui.notificationCount = 0;
    invalidateFrame(ui.frameId);
}

bool isNotificationShadeOpen(void) {
    return ui.shadeOpen;
}

void openNotificationShade(void) {
    if (!ui.initialized) return;
    freezeTopWindowIfNeeded();
    ui.shadeClosing = false;
    ui.shadeOpen = true;
    createTween(&ui.shadeReveal, (float)notificationShadeHeight(), 180, EASE_OUT_QUAD);
    invalidateFrame(ui.frameId);
}

void closeNotificationShade(void) {
    if (!ui.initialized) return;
    ui.shadeClosing = true;
    ui.shadeOpen = false;
    createTween(&ui.shadeReveal, (float)systemStatusBarHeight(), 180, EASE_OUT_QUAD);
    invalidateFrame(ui.frameId);
}

int getSystemUIFrozenWindowId(void) {
    return ui.frozenWindowId;
}

bool isSystemUICompositeActive(void) {
    int statusBarHeight = systemStatusBarHeight();
    return ui.shadeDragging || ui.shadeOpen || ui.shadeClosing || ui.shadeReveal > statusBarHeight + 0.5f;
}
