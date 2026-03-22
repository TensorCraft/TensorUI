#include "Snake.h"
#include "../../../../TensorUI/WindowManager.h"
#include "../../../../hal/screen/screen.h"
#include "../../../../hal/rand/rand.h"
#include "../../../../hal/time/time.h"
#include <stdlib.h>

#define GRID_SIZE 15
#define MAX_SNAKE_LEN 100

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;

typedef struct {
  int x, y;
} Point;

typedef struct {
  Point body[MAX_SNAKE_LEN];
  int length;
  Point food;
  Direction dir;
  bool gameOver;
  int moveCounter;
  int score;
  int frameId;
  int contentLeft;
  int contentTop;
  int contentWidth;
  int contentHeight;
  int cols;
  int rows;
  long long lastStepMs;
} SnakeGame;

static SnakeGame game = { .frameId = -1 };

static void refreshSnakeBounds() {
  ScreenInsets insets = getScreenSafeInsets();
  game.contentLeft = insets.left;
  game.contentTop = insets.top;
  game.contentWidth = getScreenContentWidth(insets);
  game.contentHeight = getScreenContentHeight(insets);
  game.cols = game.contentWidth / GRID_SIZE;
  game.rows = game.contentHeight / GRID_SIZE;
  if (game.cols < 4)
    game.cols = 4;
  if (game.rows < 4)
    game.rows = 4;
}

static void spawnFood() {
  game.food.x = (int)(hal_rand() % game.cols);
  game.food.y = (int)(hal_rand() % game.rows);
}

static void initSnake() {
  refreshSnakeBounds();
  game.length = 3;
  int startX = game.cols / 2;
  int startY = game.rows / 2;
  for (int i = 0; i < game.length; i++) {
    game.body[i].x = startX - i;
    game.body[i].y = startY;
  }
  game.dir = DIR_RIGHT;
  game.gameOver = false;
  game.score = 0;
  game.moveCounter = 0;
  game.lastStepMs = current_timestamp_ms();
  spawnFood();
}

static Color snakeGetPixel(void *self, int x, int y) {
  int right = game.contentLeft + game.cols * GRID_SIZE;
  int bottom = game.contentTop + game.rows * GRID_SIZE;

  if (x < game.contentLeft || x >= right || y < game.contentTop || y >= bottom)
    return COLOR_TRANSPARENT;

  int localX = x - game.contentLeft;
  int localY = y - game.contentTop;
  int gx = localX / GRID_SIZE;
  int gy = localY / GRID_SIZE;
  int px = localX % GRID_SIZE;
  int py = localY % GRID_SIZE;

  // Borders
  if (x == game.contentLeft || x == right - 1 || y == game.contentTop || y == bottom - 1)
    return M3_OUTLINE;

  // Draw Food
  if (gx == game.food.x && gy == game.food.y) {
    int cx = GRID_SIZE / 2;
    int cy = GRID_SIZE / 2;
    if ((px - cx) * (px - cx) + (py - cy) * (py - cy) <
        (GRID_SIZE / 2) * (GRID_SIZE / 2))
      return M3_ERROR;
  }

  // Draw Snake
  for (int i = 0; i < game.length; i++) {
    if (gx == game.body[i].x && gy == game.body[i].y) {
      if (i == 0)
        return M3_PRIMARY;
      return M3_SECONDARY;
    }
  }


  return COLOR_TRANSPARENT;
}

static void snakePreRender(void *self) {
  if (game.gameOver) {
    invalidateFrame(game.frameId);
    return;
  }

  long long now = current_timestamp_ms();
  if (now - game.lastStepMs < 140)
    return;
  game.lastStepMs = now;

  Point nextHead = game.body[0];
  if (game.dir == DIR_UP)
    nextHead.y--;
  else if (game.dir == DIR_DOWN)
    nextHead.y++;
  else if (game.dir == DIR_LEFT)
    nextHead.x--;
  else if (game.dir == DIR_RIGHT)
    nextHead.x++;

  if (nextHead.x < 0 || nextHead.x >= game.cols ||
      nextHead.y < 0 || nextHead.y >= game.rows) {
    game.gameOver = true;
    return;
  }

  for (int i = 0; i < game.length; i++) {
    if (nextHead.x == game.body[i].x && nextHead.y == game.body[i].y) {
      game.gameOver = true;
      return;
    }
  }

  bool ate = (nextHead.x == game.food.x && nextHead.y == game.food.y);
  for (int i = game.length - 1; i > 0; i--) {
    game.body[i] = game.body[i - 1];
  }
  game.body[0] = nextHead;

  if (ate) {
    if (game.length < MAX_SNAKE_LEN)
      game.length++;
    game.score += 10;
    spawnFood();
  }

  invalidateFrame(game.frameId);
}

static void snakeOnClick(void *self) {
  if (game.gameOver) {
    initSnake();
    return;
  }

  int tx, ty;
  getTouchState(&tx, &ty);
  if (tx < 40)
    return;

  int cx = game.contentLeft + game.contentWidth / 2;
  int cy = game.contentTop + game.contentHeight / 2;
  int dx = tx - cx;
  int dy = ty - cy;

  if (abs(dx) > abs(dy)) {
    if (dx > 0) {
      if (game.dir != DIR_LEFT)
        game.dir = DIR_RIGHT;
    } else {
      if (game.dir != DIR_RIGHT)
        game.dir = DIR_LEFT;
    }
  } else {
    if (dy > 0) {
      if (game.dir != DIR_UP)
        game.dir = DIR_DOWN;
    } else {
      if (game.dir != DIR_DOWN)
        game.dir = DIR_UP;
    }
  }
}

static void snakeOnDestroy(void *self) {
  game.frameId = -1;
}

void pushSnakeApp() {
  initSnake();
  if (game.frameId == -1) {
    game.frameId =
        requestFrame(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, NULL, snakePreRender,
                     snakeGetPixel, snakeOnClick, NULL);
    Frames[game.frameId].bgcolor = M3_DARK_BG;
    Frames[game.frameId].continuousRender = true;
    Frames[game.frameId].onDestroy = snakeOnDestroy;
    configureFrameAsAppSurface(game.frameId, true);
  }
  pushWindow(game.frameId);
}
