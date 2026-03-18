#include "Dialog.h"
#include "../../hal/str/str.h"
#include "../../hal/mem/mem.h"
#include "../../hal/screen/screen.h"
#include "../Card/Card.h"
#include "../Label/Label.h"
#include "../Button/Button.h"

static Color overlayGetPixel(void *self, int x, int y) {
    // Dark semi-transparent simulation
    // Since we don't have alpha, we'll just use a very dark color
    return (Color){10, 10, 10, false};
}

void closeDialog(Dialog *dialog) {
    int previousFocusFrameId = dialog->previousFocusFrameId;
    TextInputTarget previousTextInputTarget = dialog->previousTextInputTarget;
    cancelAllFrameInteractions();
    // We only need to destroy the overlay frame now, 
    // it will recursively destroy the card and children.
    destroyFrame(dialog->overlayFrameId);
    if (previousTextInputTarget.frameId >= 0 &&
        previousTextInputTarget.frameId < MAX_FRAMES &&
        Frames[previousTextInputTarget.frameId].enabled) {
        registerTextInputTarget(previousTextInputTarget);
    } else if (previousFocusFrameId >= 0 &&
               previousFocusFrameId < MAX_FRAMES &&
               Frames[previousFocusFrameId].enabled) {
        setFocusedFrame(previousFocusFrameId);
    }
    renderFlag = true;
}

static void dialogOnDestroy(void *self) {
    Dialog *d = (Dialog *)self;
    if (d->title) hal_free(d->title);
    if (d->message) hal_free(d->message);
    if (d->positiveText) hal_free(d->positiveText);
    if (d->negativeText) hal_free(d->negativeText);
    hal_free(d);
}

static void onPosInternal(void *arg) {
    Dialog *d = (Dialog *)arg;
    if (d->onPositive) d->onPositive(d->arg);
    closeDialog(d);
}

static void onNegInternal(void *arg) {
    Dialog *d = (Dialog *)arg;
    if (d->onNegative) d->onNegative(d->arg);
    closeDialog(d);
}

Dialog* createDialog(char *title, char *message, char *posText, char *negText, Font tFont, Font mFont, void (*onPos)(void*), void (*onNeg)(void*), void* arg) {
    Dialog *d = (Dialog *)hal_malloc(sizeof(Dialog));
    d->title = hal_strdup(title);
    d->message = hal_strdup(message);
    d->positiveText = hal_strdup(posText);
    d->negativeText = negText ? hal_strdup(negText) : NULL;
    d->onPositive = onPos;
    d->onNegative = onNeg;
    d->arg = arg;
    d->previousFocusFrameId = -1;
    d->previousTextInputTarget = (TextInputTarget){ .frameId = -1 };

    // Overlay
    d->overlayFrameId = requestFrame(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, d, NULL, overlayGetPixel, NULL, NULL);
    Frames[d->overlayFrameId].enabled = false;
    configureFrameAsSystemSurface(d->overlayFrameId, true);
    Frames[d->overlayFrameId].onDestroy = dialogOnDestroy;

    // Card (Dialog body)
    int dw = 224;
    int dh = 160;
    int dx = (SCREEN_WIDTH - dw) / 2;
    int dy = (SCREEN_HEIGHT - dh) / 2;
    
    Card* card = createCard(dx, dy, dw, dh, M3_SURFACE);
    d->frameId = card->frameId;
    setCardContentInsets(card, 16, 12, 12, 12);
    setVStackScrollbar(card->content, true); // Enable scrolling for long content

    Frames[d->frameId].enabled = false;
    configureFrameAsSystemSurface(d->frameId, true);
    Frames[d->frameId].parentId = d->overlayFrameId;

    Frames[d->frameId].parentId = d->overlayFrameId;

    // Content
    int contentW = dw - 24;
    Label *tLabel = createLabel(0, 0, contentW, 30, title, M3_PRIMARY, COLOR_TRANSPARENT, tFont);
    addFrameToCard(card, tLabel->frameId);

    Label *mLabel = createLabel(0, 0, contentW, 64, message, M3_ON_SURFACE, COLOR_TRANSPARENT, mFont);
    mLabel->isMultiLine = true;
    mLabel->wrapWidth = contentW;
    addFrameToCard(card, mLabel->frameId);

    HStack *btnRow = createHStack(0, 0, contentW, 44, 8);
    Frames[btnRow->frameId].alignment = ALIGNMENT_RIGHT;
    addFrameToCard(card, btnRow->frameId);

    if (negText) {
        Button *btnNeg = createButton(0, 0, 70, 32, negText, M3_PRIMARY, M3_SURFACE_VARIANT, M3_OUTLINE, mFont, onNegInternal, d);
        addFrameToHStack(btnRow, btnNeg->frameId);
    }

    Button *btnPos = createButton(0, 0, 70, 32, posText, M3_ON_PRIMARY, M3_PRIMARY, M3_PRIMARY, mFont, onPosInternal, d);
    addFrameToHStack(btnRow, btnPos->frameId);


    return d;
}

void showDialog(Dialog *dialog) {
    dialog->previousFocusFrameId = focus;
    dialog->previousTextInputTarget = currentTextInputTarget;
    cancelAllFrameInteractions();
    clearTextInputTarget();
    clearFocusedFrame();
    Frames[dialog->overlayFrameId].enabled = true;
    Frames[dialog->frameId].enabled = true;
    renderFlag = true;
}
