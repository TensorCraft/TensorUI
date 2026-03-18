#ifndef SYSTEM_UI_H
#define SYSTEM_UI_H

#include "../Font/font.h"
#include "../Color/color.h"
#include <stdbool.h>

typedef void (*SystemNotificationClickHandler)(void *context);

void initSystemUI(Font statusFont, Font bodyFont);
void updateSystemUI(void);

void setStatusBarTitle(const char *title);
void setStatusBarClockText(const char *clockText);
void setStatusBarIcon(int slot, const Color *pixels, int srcW, int srcH, int size);
void clearStatusBarIcon(int slot);

void pushSystemNotification(const char *title, const char *message);
void pushSystemNotificationWithAction(const char *title, const char *message,
                                      SystemNotificationClickHandler onClick, void *context);
void clearSystemNotifications(void);

bool isNotificationShadeOpen(void);
void openNotificationShade(void);
void closeNotificationShade(void);
int getSystemUIFrozenWindowId(void);
bool isSystemUICompositeActive(void);

#endif
