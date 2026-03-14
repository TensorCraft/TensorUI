#include "Snake.h"
#include "../../../../TensorUI/WindowManager.h"
#include "../../../../hal/screen/screen.h"
#include <stdlib.h>
#include <time.h>

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
} SnakeGame;

static SnakeGame game;

static void spawnFood() {
  game.food.x = rand() % (SCREEN_WIDTH / GRID_SIZE);
  game.food.y = rand() % (SCREEN_HEIGHT / GRID_SIZE);
}

static void initSnake() {
  game.length = 3;
  int startX = (SCREEN_WIDTH / GRID_SIZE) / 2;
  int startY = (SCREEN_HEIGHT / GRID_SIZE) / 2;
  for (int i = 0; i < game.length; i++) {
    game.body[i].x = startX - i;
    game.body[i].y = startY;
  }
  game.dir = DIR_RIGHT;
  game.gameOver = false;
  game.score = 0;
  game.moveCounter = 0;
  spawnFood();
}

static Color snakeGetPixel(void *self, int x, int y) {
  int gx = x / GRID_SIZE;
  int gy = y / GRID_SIZE;
  int px = x % GRID_SIZE;
  int py = y % GRID_SIZE;

  // Borders
  if (x == 0 || x == SCREEN_WIDTH - 1 || y == 0 || y == SCREEN_HEIGHT - 1)
    return COLOR_DARK_GRAY;

  // Draw Food
  if (gx == game.food.x && gy == game.food.y) {
    int cx = GRID_SIZE / 2;
    int cy = GRID_SIZE / 2;
    if ((px - cx) * (px - cx) + (py - cy) * (py - cy) <
        (GRID_SIZE / 2) * (GRID_SIZE / 2))
      return COLOR_RED;
  }

  // Draw Snake
  for (int i = 0; i < game.length; i++) {
    if (gx == game.body[i].x && gy == game.body[i].y) {
      if (i == 0)
        return COLOR_CYAN;
      return COLOR_GREEN;
    }
  }

  return COLOR_TRANSPARENT;
}

static void snakePreRender(void *self) {
  if (game.gameOver) {
    renderFlag = true;
    return;
  }

  game.moveCounter++;
  if (game.moveCounter < 1500)
    return;
  game.moveCounter = 0;

  Point nextHead = game.body[0];
  if (game.dir == DIR_UP)
    nextHead.y--;
  else if (game.dir == DIR_DOWN)
    nextHead.y++;
  else if (game.dir == DIR_LEFT)
    nextHead.x--;
  else if (game.dir == DIR_RIGHT)
    nextHead.x++;

  if (nextHead.x < 0 || nextHead.x >= (SCREEN_WIDTH / GRID_SIZE) ||
      nextHead.y < 0 || nextHead.y >= (SCREEN_HEIGHT / GRID_SIZE)) {
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

  renderFlag = true;
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

  int cx = SCREEN_WIDTH / 2;
  int cy = SCREEN_HEIGHT / 2;
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

void pushSnakeApp() {
  initSnake();
  game.frameId =
      requestFrame(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, NULL, snakePreRender,
                   snakeGetPixel, snakeOnClick, NULL);
  Frames[game.frameId].bgcolor = COLOR_BLACK;
  pushWindow(game.frameId);
}
