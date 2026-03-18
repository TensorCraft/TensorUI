#ifndef CARD_H
#define CARD_H

#include "../VStack/VStack.h"
#include "../Color/color.h"

typedef struct {
    VStack *content;
    Color bgColor;
    int borderRadius;
    int contentInsetTop;
    int contentInsetRight;
    int contentInsetBottom;
    int contentInsetLeft;
    int frameId;
} Card;

Card* createCard(int x, int y, int w, int h, Color bgColor);
void addFrameToCard(Card *card, int frameId);
void setCardCornerRadius(Card *card, int radius);
void setCardContentInsets(Card *card, int top, int right, int bottom, int left);

#endif
