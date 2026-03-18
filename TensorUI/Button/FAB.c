#include "FAB.h"
#include "../../hal/math/math.h"
#include "../../hal/mem/mem.h"
#include "../../hal/str/str.h"
#include "../../hal/screen/screen.h"

static Color fabGetPixel(void *self, int x, int y) {
    FAB *fab = (FAB *)self;
    int r = fab->radius;
    float dist = hal_sqrtf(((x - r)*(x - r)) + ((y - r)*(y - r)));
    
    if (dist > r) return COLOR_TRANSPARENT;
    
    // Draw icon (very simple + sign if icon is "+")
    if (fab->icon && hal_strcmp(fab->icon, "+") == 0) {
        int mid = r;
        if ((x >= mid - 1 && x <= mid + 1 && y >= r/2 && y <= 2*r - r/2) ||
            (y >= mid - 1 && y <= mid + 1 && x >= r/2 && x <= 2*r - r/2)) {
            return fab->iconColor;
        }
    }

    // Shadow-like edge or just AA
    if (dist > r - 1) {
        float ratio = dist - (r - 1);
        return (Color){fab->bgColor.r * (1.0-ratio), fab->bgColor.g * (1.0-ratio), fab->bgColor.b * (1.0-ratio), false};
    }

    return fab->bgColor;
}

static void fabOnClickInternal(void *self) {
    FAB *fab = (FAB *)self;
    if (fab->onClick) fab->onClick(fab->arg);
}

FAB* createFAB(int x, int y, char *icon, Color bgColor, Color iconColor, void (*onClick)(void*), void* arg) {
    FAB *fab = (FAB *)hal_malloc(sizeof(FAB));
    fab->radius = 28;
    fab->x = x;
    fab->y = y;
    fab->icon = hal_strdup(icon);
    fab->bgColor = bgColor;
    fab->iconColor = iconColor;
    fab->onClick = onClick;
    fab->arg = arg;

    fab->frameId = requestFrame(fab->radius * 2, fab->radius * 2, x, y, fab, NULL, fabGetPixel, fabOnClickInternal, NULL);
    return fab;
}
