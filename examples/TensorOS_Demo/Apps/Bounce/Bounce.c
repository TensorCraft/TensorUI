#include "Bounce.h"
#include "../../../../TensorUI/WindowManager.h"
#include "../../../../hal/time/time.h"
#include "../../../../hal/rand/rand.h"
#include "../../../../hal/math/math.h"
#include "../../../../hal/screen/screen.h"

#define BRICK_COLS 6
#define BRICK_ROWS 3

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
} Ball;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} Paddle;

typedef struct {
    bool alive;
    int x;
    int y;
    int w;
    int h;
} Brick;

typedef struct {
    int frameId;
    int contentLeft;
    int contentTop;
    int contentRight;
    int contentBottom;
    int contentWidth;
    int contentHeight;
    Ball ball;
    Paddle paddle;
    Brick bricks[BRICK_ROWS][BRICK_COLS];
    long long lastTick;
} BounceGame;

static BounceGame game = { .frameId = -1 };

static void refreshBounds() {
    ScreenInsets insets = getScreenSafeInsets();
    game.contentLeft = insets.left;
    game.contentTop = insets.top;
    game.contentWidth = getScreenContentWidth(insets);
    game.contentHeight = getScreenContentHeight(insets);
    game.contentRight = game.contentLeft + game.contentWidth;
    game.contentBottom = game.contentTop + game.contentHeight;
}

static void resetBricks() {
    int padding = 8;
    int top = game.contentTop + 18;
    int gap = 4;
    int totalGapW = gap * (BRICK_COLS - 1);
    int brickW = (game.contentWidth - 2 * padding - totalGapW) / BRICK_COLS;
    int brickH = 8;
    int startX = game.contentLeft + padding;

    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            Brick *b = &game.bricks[r][c];
            b->alive = true;
            b->w = brickW;
            b->h = brickH;
            b->x = startX + c * (brickW + gap);
            b->y = top + r * (brickH + gap);
        }
    }
}

static void resetBall() {
    game.ball.x = game.contentLeft + game.contentWidth / 2.0f;
    game.ball.y = game.contentTop + game.contentHeight / 2.0f;
    game.ball.vx = (hal_rand() % 2 == 0) ? -0.10f : 0.10f;
    game.ball.vy = -0.16f;
}

static void resetGame() {
    refreshBounds();
    game.paddle.w = 48;
    game.paddle.h = 6;
    game.paddle.y = game.contentBottom - 18;
    game.paddle.x = game.contentLeft + (game.contentWidth - game.paddle.w) / 2.0f;
    resetBricks();
    resetBall();
    game.lastTick = current_timestamp_ms();
}

static void updatePaddle() {
    int tx, ty;
    getTouchState(&tx, &ty);
    float target = (float)tx - game.paddle.w / 2.0f;
    if (target < game.contentLeft + 6) target = game.contentLeft + 6;
    if (target > game.contentRight - game.paddle.w - 6) target = game.contentRight - game.paddle.w - 6;
    game.paddle.x = target;
}

static void bounceBall(float dt) {
    game.ball.x += game.ball.vx * dt;
    game.ball.y += game.ball.vy * dt;

    // Walls
    if (game.ball.x < game.contentLeft + 4) { game.ball.x = game.contentLeft + 4; game.ball.vx = -game.ball.vx; }
    if (game.ball.x > game.contentRight - 4) { game.ball.x = game.contentRight - 4; game.ball.vx = -game.ball.vx; }
    if (game.ball.y < game.contentTop + 6) { game.ball.y = game.contentTop + 6; game.ball.vy = -game.ball.vy; }

    // Paddle
    float px = game.paddle.x;
    float py = game.paddle.y;
    float pw = game.paddle.w;
    float ph = game.paddle.h;
    if (game.ball.y + 3 >= py && game.ball.y + 3 <= py + ph) {
        if (game.ball.x >= px && game.ball.x <= px + pw) {
            float hit = (game.ball.x - (px + pw / 2.0f)) / (pw / 2.0f);
            game.ball.vx = hit * 0.18f;
            game.ball.vy = -0.18f;
            game.ball.y = py - 4;
        }
    }

    // Bricks
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            Brick *b = &game.bricks[r][c];
            if (!b->alive) continue;
            if (game.ball.x + 3 >= b->x && game.ball.x - 3 <= b->x + b->w &&
                game.ball.y + 3 >= b->y && game.ball.y - 3 <= b->y + b->h) {
                b->alive = false;
                game.ball.vy = -game.ball.vy;
                return;
            }
        }
    }

    // Missed
    if (game.ball.y > game.contentBottom + 10) {
        resetGame();
    }
}

static Color bounceGetPixel(void *self, int x, int y) {
    // Background
    Color bg = M3_DARK_BG;

    // Bricks
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            Brick *b = &game.bricks[r][c];
            if (!b->alive) continue;
            if (x >= b->x && x < b->x + b->w && y >= b->y && y < b->y + b->h) {
                return (Color){80 + c * 15, 100 + r * 25, 200, false};
            }
        }
    }

    // Paddle
    if (x >= (int)game.paddle.x && x < (int)(game.paddle.x + game.paddle.w) &&
        y >= (int)game.paddle.y && y < (int)(game.paddle.y + game.paddle.h)) {
        return (Color){200, 180, 255, false};
    }

    // Ball
    int bx = (int)game.ball.x;
    int by = (int)game.ball.y;
    int dx = x - bx;
    int dy = y - by;
    if (dx*dx + dy*dy <= 9) {
        return (Color){110, 168, 255, false};
    }

    return bg;
}

static void bouncePreRender(void *self) {
    long long now = current_timestamp_ms();
    long long dt = now - game.lastTick;
    if (dt < 1) return;
    if (dt > 40) dt = 40;
    game.lastTick = now;

    updatePaddle();
    bounceBall((float)dt);
    invalidateFrame(game.frameId);
}

static void bounceOnDestroy(void *self) {
    game.frameId = -1;
}

void pushBounceApp() {
    resetGame();
    if (game.frameId == -1) {
        game.frameId = requestFrame(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, NULL,
                                    bouncePreRender, bounceGetPixel, NULL, NULL);
        Frames[game.frameId].bgcolor = M3_DARK_BG;
        Frames[game.frameId].continuousRender = true;
        Frames[game.frameId].onDestroy = bounceOnDestroy;
        configureFrameAsAppSurface(game.frameId, true);
    }
    pushWindow(game.frameId);
}
