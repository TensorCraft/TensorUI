#include "Card.h"
#include "../../hal/math/math.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"

static Color cardGetPixel(void *self, int x, int y) {
    Card *card = (Card *)self;
    int w = Frames[card->frameId].width;
    int h = Frames[card->frameId].height;
    int r = card->borderRadius;

    // Faster AA: Check corners using squared distance
    int cx, cy;
    bool inCorner = false;
    if (x < r && y < r) { cx = r; cy = r; inCorner = true; }
    else if (x >= w-r && y < r) { cx = w-r-1; cy = r; inCorner = true; }
    else if (x < r && y >= h-r) { cx = r; cy = h-r-1; inCorner = true; }
    else if (x >= w-r && y >= h-r) { cx = w-r-1; cy = h-r-1; inCorner = true; }

    if (inCorner) {
        int dx = x - cx;
        int dy = y - cy;
        int d2 = dx*dx + dy*dy;
        int r2 = r*r;
        if (d2 > r2) return COLOR_TRANSPARENT;
        
        // Edge Softening (1px)
        if (d2 > (r-1)*(r-1)) {
            float dist = hal_sqrtf((float)d2);
            float ratio = dist - (r - 1);
            return (Color){card->bgColor.r * (1.0f - ratio), card->bgColor.g * (1.0f - ratio), card->bgColor.b * (1.0f - ratio), false};
        }
    }

    // Material 3 Flat Style: No border, just surface elevation
    return card->bgColor;
}

static void cardOnDestroy(void *self) {
    Card *card = (Card *)self;
    // The child content (VStack) will be destroyed by the recursive 
    // destroyFrame logic in screen.c. We just free the Card object.
    hal_free(card);
}

static void cardPreRender(void *self) {
    Card *card = (Card *)self;
    int px = Frames[card->frameId].x;
    int py = Frames[card->frameId].y;
    int contentWidth = Frames[card->frameId].width - card->contentInsetLeft - card->contentInsetRight;
    int contentHeight = Frames[card->frameId].height - card->contentInsetTop - card->contentInsetBottom;

    Frames[card->content->frameId].x = px + card->contentInsetLeft;
    Frames[card->content->frameId].y = py + card->contentInsetTop;
    Frames[card->content->frameId].fx = (float)Frames[card->content->frameId].x;
    Frames[card->content->frameId].fy = (float)Frames[card->content->frameId].y;
    Frames[card->content->frameId].width = contentWidth > 1 ? contentWidth : 1;
    Frames[card->content->frameId].height = contentHeight > 1 ? contentHeight : 1;

    // Centralized compositor now handles overflow/clipping
}

Card* createCard(int x, int y, int w, int h, Color bgColor) {
    Card *card = (Card *)hal_malloc(sizeof(Card));
    card->bgColor = bgColor;
    card->borderRadius = 16; // Material 3 uses larger rounded corners
    card->contentInsetTop = 12;
    card->contentInsetRight = 12;
    card->contentInsetBottom = 12;
    card->contentInsetLeft = 12;

    card->content = createVStack(x + card->contentInsetLeft, y + card->contentInsetTop,
                                 w - card->contentInsetLeft - card->contentInsetRight,
                                 h - card->contentInsetTop - card->contentInsetBottom, 8);
    
    card->frameId = requestFrame(w, h, x, y, card, cardPreRender, cardGetPixel, NULL, NULL);
    Frames[card->frameId].bgcolor = COLOR_TRANSPARENT;
    Frames[card->frameId].overflowX = OVERFLOW_HIDDEN;
    Frames[card->frameId].overflowY = OVERFLOW_HIDDEN;
    Frames[card->frameId].onDestroy = cardOnDestroy;
    Frames[card->content->frameId].parentId = card->frameId;
    
    return card;
}

void addFrameToCard(Card *card, int id) {
    addFrameToVStack(card->content, id);
}

void setCardCornerRadius(Card *card, int radius) {
    if (!card) return;
    card->borderRadius = radius;
    renderFlag = true;
}

void setCardContentInsets(Card *card, int top, int right, int bottom, int left) {
    if (!card) return;
    card->contentInsetTop = top;
    card->contentInsetRight = right;
    card->contentInsetBottom = bottom;
    card->contentInsetLeft = left;
    renderFlag = true;
}
