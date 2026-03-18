#include "ListTile.h"
#include "../../hal/str/str.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"
#include "../Label/Label.h"

typedef struct {
    int titleFrameId;
    int subtitleFrameId;
} ListTileInternal;

static Color listTileGetPixel(void *self, int x, int y) {
    ListTile *lt = (ListTile *)self;
    // Material divider: subtle line at the bottom
    if (y >= lt->height - 2) return (Color){45, 45, 45, false};
    return COLOR_TRANSPARENT;
}

static void listTilePreRender(void *self) {
    ListTile *lt = (ListTile *)self;
    ListTileInternal *internal = (ListTileInternal *)lt->arg;
    
    // ListTile position might change (e.g. in a scrolling VStack)
    // We must update children accordingly
    int px = Frames[lt->frameId].x;
    int py = Frames[lt->frameId].y;

    if (internal->titleFrameId != -1) {
        Frames[internal->titleFrameId].x = px + 15;
        Frames[internal->titleFrameId].y = py + 8;
        Frames[internal->titleFrameId].clipX = Frames[lt->frameId].clipX;
        Frames[internal->titleFrameId].clipY = Frames[lt->frameId].clipY;
        Frames[internal->titleFrameId].clipW = Frames[lt->frameId].clipW;
        Frames[internal->titleFrameId].clipH = Frames[lt->frameId].clipH;
    }
    if (internal->subtitleFrameId != -1) {
        Frames[internal->subtitleFrameId].x = px + 15;
        Frames[internal->subtitleFrameId].y = py + 32;
        Frames[internal->subtitleFrameId].clipX = Frames[lt->frameId].clipX;
        Frames[internal->subtitleFrameId].clipY = Frames[lt->frameId].clipY;
        Frames[internal->subtitleFrameId].clipW = Frames[lt->frameId].clipW;
        Frames[internal->subtitleFrameId].clipH = Frames[lt->frameId].clipH;
    }
}

static void listTileOnClickInternal(void *self) {
    ListTile *lt = (ListTile *)self;
    if (lt->onClick) lt->onClick(lt->arg_user);
}

ListTile* createListTile(int x, int y, int w, char *title, char *subtitle, Font tFont, Font sFont, void (*onClick)(void*), void* arg) {
    ListTile *lt = (ListTile *)hal_malloc(sizeof(ListTile));
    lt->x = x;
    lt->y = y;
    lt->width = w;
    lt->height = subtitle ? 60 : 48; // Correct Material height
    lt->title = hal_strdup(title);
    lt->subtitle = subtitle ? hal_strdup(subtitle) : NULL;
    lt->titleFont = tFont;
    lt->subtitleFont = sFont;
    lt->titleColor = COLOR_WHITE;
    lt->subtitleColor = (Color){150, 150, 150, false};
    lt->onClick = onClick;
    lt->arg_user = arg; // Renamed to avoid confusion with internal arg

    ListTileInternal *internal = (ListTileInternal *)hal_malloc(sizeof(ListTileInternal));
    lt->arg = internal;

    lt->frameId = requestFrame(lt->width, lt->height, x, y, lt, listTilePreRender, listTileGetPixel, listTileOnClickInternal, NULL);
    
    // Create internal labels
    Label *tLabel = createLabel(x + 15, y + 8, w - 30, 24, title, lt->titleColor, COLOR_TRANSPARENT, tFont);
    Frames[tLabel->frameId].parentId = lt->frameId;
    internal->titleFrameId = tLabel->frameId;
    
    if (subtitle) {
        Label *sLabel = createLabel(x + 15, y + 32, w - 30, 20, subtitle, lt->subtitleColor, COLOR_TRANSPARENT, sFont);
        Frames[sLabel->frameId].parentId = lt->frameId;
        internal->subtitleFrameId = sLabel->frameId;
    } else {
        internal->subtitleFrameId = -1;
    }

    return lt;
}

