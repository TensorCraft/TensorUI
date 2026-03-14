#include "../../TensorUI/Button/Button.h"
#include "../../TensorUI/Canvas/Canvas.h"
#include "../../TensorUI/Font/font.h"
#include "../../TensorUI/Label/Label.h"
#include "../../TensorUI/ProgressBar/ProgressBar.h"
#include "../../TensorUI/ProgressBar/Slider.h"
#include "../../TensorUI/SwitchTab/SwitchTab.h"
#include "../../TensorUI/SwitchTab/Toggle.h"
#include "../../TensorUI/Toast/Toast.h"
#include "../../TensorUI/VStack/VStack.h"
#include "../../TensorUI/WindowManager.h"
#include "../../hal/screen/screen.h"
#include "../../hal/time/time.h"
#include "Apps/Calculator/Calculator.h"
#include "Apps/Snake/Snake.h"
#include <SDL2/SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern Frame Frames[];
extern int frameCount;
extern bool renderFlag;

Font font_22;
Font font_18;
Font font_12;

int currentTab = 0;
VStack *homePage = NULL;
VStack *settingsPage = NULL;
VStack *aboutPage = NULL;
VStack *drawPage = NULL;

int homeFrames[50];
int homeCount = 0;
int settingsFrames[50];
int settingsCount = 0;
int aboutFrames[50];
int aboutCount = 0;
int drawFrames[50];
int drawCount = 0;

Canvas *demoCanvas = NULL;

void onSliderChange(float val) { showToast("Adjusting Brightness...", 800); }

void onClearClick(void *self) {
  if (demoCanvas)
    clearCanvas(demoCanvas);
  showToast("Canvas Cleared!", 1500);
}

void onRedClick(void *self) {
  if (demoCanvas)
    setCanvasColor(demoCanvas, (Color){255, 60, 60, false});
  showToast("Color: Red", 1000);
}

void onCyanClick(void *self) {
  if (demoCanvas)
    setCanvasColor(demoCanvas, COLOR_CYAN);
  showToast("Color: Cyan", 1000);
}

void onEraserClick(void *self) {
  if (demoCanvas)
    setCanvasColor(demoCanvas, (Color){20, 20, 20, false});
  showToast("Eraser Selected", 1000);
}

void onLaunchSettings(void *self) {
  pushWindow(settingsPage->frameId);
  showToast("Launching Settings", 800);
}

void onLaunchAbout(void *self) {
  pushWindow(aboutPage->frameId);
  showToast("Launching System", 800);
}

void onLaunchDraw(void *arg) { pushWindow(drawPage->frameId); }

void onLaunchSnake(void *arg) { pushSnakeApp(); }

void onLaunchCalc(void *arg) { pushCalculatorApp(); }

/**
 * PAGE SETUP FUNCTIONS
 */
void onQueueToastClick(void *self) {
  showToast("Toast 1: Hello from TensorOS!", 2000);
  showToast("Toast 2: I'm in a queue!", 2000);
  showToast("Toast 3: One by one...", 2000);
}

void onAAToggle(void *self, bool state) {
  setAntiAliasing(state);
  if (state)
    showToast("Anti-Aliasing: ON", 1500);
  else
    showToast("Anti-Aliasing: OFF", 1500);
}

static void onAAToggleWrapper(bool state) { onAAToggle(NULL, state); }

void onTabChange(int index) {
  // Redundant in Window Stack mode but kept for compat if needed
}

int main() {
  init_screen("TensorOS Premium", SCREEN_WIDTH, SCREEN_HEIGHT);

  font_22 = loadFont("./fonts/LiberationSans-Regular22.bfont");
  font_18 = loadFont("./fonts/LiberationSans-Regular18.bfont");
  font_12 = loadFont("./fonts/LiberationSans-Regular12.bfont");

  // ==========================================
  // BOOT SEQUENCE
  // ==========================================
  HStack *logoStack =
      createHStack(0, SCREEN_HEIGHT / 2 - 40, SCREEN_WIDTH, 40, 0);
  Frames[logoStack->frameId].alignment =
      ALIGNMENT_CENTER_HORIZONTAL | ALIGNMENT_CENTER_VERTICAL;

  createLabel(0, 0, 0, 0, "Tensor", COLOR_CYAN, COLOR_TRANSPARENT, font_22);
  addFrameToHStack(logoStack, frameCount - 1);

  createLabel(0, 0, 0, 0, "OS", COLOR_WHITE, COLOR_TRANSPARENT, font_22);
  addFrameToHStack(logoStack, frameCount - 1);

  ProgressBar *bootProgress = createProgressBar(
      SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 20, 200, 10, 0.0f);

  long bootStart = current_timestamp_ms();
  while (true) {
    long elapsed = current_timestamp_ms() - bootStart;
    float p = elapsed / 2500.0f; // 2.5 seconds boot
    p = p * p;                   // non-linear progression
    if (p > 1.0f)
      p = 1.0f;

    setProgress(bootProgress, p);

    if (!updateScreen())
      return 0;

    if (p >= 1.0f) {
      SDL_Delay(500); // pause at 100%
      break;
    }
  }

  // Clear Boot Screen explicitly before making home screen
  for (int i = 0; i < frameCount; i++)
    Frames[i].enabled = false;
  renderFlag = true;
  fillScreen(COLOR_BLACK); // clear out old buffer

  // ==========================================
  // INITIALIZE MANAGER
  // ==========================================
  initWindowManager();

  // ==========================================
  // HOME SCREEN (NAVIGATION HUB)
  // ==========================================
  homePage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 15);
  Frames[homePage->frameId].paddingTop = 5; // Space for title
  Frames[homePage->frameId].bgcolor = (Color){15, 15, 15, false};
  Frames[homePage->frameId].alignment |= ALIGNMENT_CENTER_HORIZONTAL;
  pushWindow(homePage->frameId);

  // SYSTEM TITLE
  // SYSTEM TITLE
  Label *titleLabel = createLabel(0, 0, 0, 0, "TensorOS", COLOR_CYAN,
                                  COLOR_TRANSPARENT, font_22);
  titleLabel->alignment = ALIGNMENT_CENTER_HORIZONTAL;
  addFrameToVStack(homePage, titleLabel->frameId);

  Label *subtitleLabel =
      createLabel(0, 0, 0, 0, "Apps:", COLOR_GRAY, COLOR_TRANSPARENT, font_12);
  subtitleLabel->alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[subtitleLabel->frameId].marginTop = 5;
  addFrameToVStack(homePage, subtitleLabel->frameId);

  // APP LIST
  createButton(0, 0, SCREEN_WIDTH - 40, 50, "System Settings", COLOR_WHITE,
               (Color){45, 45, 45, false}, (Color){70, 70, 70, false}, font_18,
               onLaunchSettings, NULL);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(homePage, frameCount - 1);

  createButton(0, 0, SCREEN_WIDTH - 40, 50, "About TensorOS", COLOR_WHITE,
               (Color){45, 45, 45, false}, (Color){70, 70, 70, false}, font_18,
               onLaunchAbout, NULL);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(homePage, frameCount - 1);

  createButton(0, 0, SCREEN_WIDTH - 40, 50, "Digital Paint", COLOR_CYAN,
               (Color){45, 45, 45, false}, (Color){70, 70, 70, false}, font_18,
               onLaunchDraw, NULL);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(homePage, frameCount - 1);

  createButton(0, 0, SCREEN_WIDTH - 40, 50, "Snake Game", COLOR_GREEN,
               (Color){45, 45, 45, false}, (Color){70, 70, 70, false}, font_18,
               onLaunchSnake, NULL);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(homePage, frameCount - 1);

  createButton(0, 0, SCREEN_WIDTH - 40, 50, "Calculator", COLOR_YELLOW,
               (Color){45, 45, 45, false}, (Color){70, 70, 70, false}, font_18,
               onLaunchCalc, NULL);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(homePage, frameCount - 1);

  // ==========================================
  // APP 1: SETTINGS (Isolated Container)
  // ==========================================
  settingsPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 15);
  Frames[settingsPage->frameId].paddingTop = 20;
  Frames[settingsPage->frameId].bgcolor = (Color){25, 25, 25, false};
  Frames[settingsPage->frameId].enabled = false; // Hidden at start

  createLabel(0, 0, 0, 0, "Display Settings", COLOR_CYAN, COLOR_TRANSPARENT,
              font_18);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(settingsPage, frameCount - 1);

  createLabel(0, 0, 0, 0, "Brightness", COLOR_LIGHT_GRAY, COLOR_TRANSPARENT,
              font_12);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(settingsPage, frameCount - 1);

  createSlider(0, 0, SCREEN_WIDTH - 40, 30, 0.75f, onSliderChange);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(settingsPage, frameCount - 1);

  createLabel(0, 0, 0, 0, "Features", COLOR_CYAN, COLOR_TRANSPARENT, font_18);
  Frames[frameCount - 1].marginLeft = 20;
  Frames[frameCount - 1].marginTop = 20;
  addFrameToVStack(settingsPage, frameCount - 1);

  char *settingNames[] = {"Wi-Fi",      "Bluetooth",   "Do Not Disturb",
                          "Location",   "Auto-Rotate", "Dark Mode",
                          "Night Shift"};
  for (int i = 0; i < 7; i++) {
    HStack *row = createHStack(0, 0, SCREEN_WIDTH - 40, 40, 0);
    Frames[row->frameId].marginLeft = 20;
    Frames[row->frameId].alignment =
        ALIGNMENT_CENTER_VERTICAL | ALIGNMENT_RIGHT; // Align toggle to right
    addFrameToVStack(settingsPage, row->frameId);

    createLabel(0, 0, SCREEN_WIDTH - 110, 30, settingNames[i], COLOR_WHITE,
                COLOR_TRANSPARENT, font_18);
    addFrameToHStack(row, frameCount - 1);

    createToggle(0, 0, i % 2 == 0, NULL);
    addFrameToHStack(row, frameCount - 1);
  }

  // Anti-Aliasing Row
  HStack *aaRow = createHStack(0, 0, SCREEN_WIDTH - 40, 40, 0);
  Frames[aaRow->frameId].marginLeft = 20;
  Frames[aaRow->frameId].alignment =
      ALIGNMENT_CENTER_VERTICAL | ALIGNMENT_RIGHT;
  addFrameToVStack(settingsPage, aaRow->frameId);

  createLabel(0, 0, SCREEN_WIDTH - 110, 30, "Anti-Aliasing",
              (Color){255, 200, 50, false}, COLOR_TRANSPARENT, font_18);
  addFrameToHStack(aaRow, frameCount - 1);

  createToggle(0, 0, false, onAAToggleWrapper);
  addFrameToHStack(aaRow, frameCount - 1);

  // ------------------------------------------
  // APP 2: SYSTEM INFO
  // ------------------------------------------
  aboutPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 15);
  Frames[aboutPage->frameId].paddingTop = 20;
  Frames[aboutPage->frameId].enabled = false;
  Frames[aboutPage->frameId].bgcolor = (Color){25, 25, 25, false};

  createLabel(0, 0, 0, 0, "General", COLOR_CYAN, COLOR_TRANSPARENT, font_18);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(aboutPage, frameCount - 1);

  createLabel(0, 0, 0, 0, "TensorOS 1.0 LTS Core Framework", COLOR_WHITE,
              COLOR_TRANSPARENT, font_12);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(aboutPage, frameCount - 1);

  createLabel(0, 0, 0, 0, "Memory Allocation", COLOR_LIGHT_GRAY,
              COLOR_TRANSPARENT, font_12);
  Frames[frameCount - 1].marginLeft = 20;
  Frames[frameCount - 1].marginTop = 20;
  addFrameToVStack(aboutPage, frameCount - 1);

  createProgressBar(0, 0, SCREEN_WIDTH - 40, 15, 0.42f);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(aboutPage, frameCount - 1);

  createLabel(0, 0, 0, 0, "Storage Pool", COLOR_LIGHT_GRAY, COLOR_TRANSPARENT,
              font_12);
  Frames[frameCount - 1].marginLeft = 20;
  Frames[frameCount - 1].marginTop = 15;
  addFrameToVStack(aboutPage, frameCount - 1);

  createProgressBar(0, 0, SCREEN_WIDTH - 40, 15, 0.88f);
  Frames[frameCount - 1].marginLeft = 20;
  addFrameToVStack(aboutPage, frameCount - 1);

  createButton(0, 0, SCREEN_WIDTH - 40, 40, "Test Toast Queue", COLOR_WHITE,
               (Color){80, 80, 80, false}, COLOR_GRAY, font_12,
               onQueueToastClick, NULL);
  Frames[frameCount - 1].marginLeft = 20;
  Frames[frameCount - 1].marginTop = 20;
  addFrameToVStack(aboutPage, frameCount - 1);

  // ------------------------------------------
  // APP 3: CANVAS DRAWING DEMO
  // ------------------------------------------
  drawPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 5);
  Frames[drawPage->frameId].paddingTop = 20;
  Frames[drawPage->frameId].enabled = false;
  Frames[drawPage->frameId].bgcolor = (Color){25, 25, 25, false};

  HStack *toolbar = createHStack(0, 0, SCREEN_WIDTH - 20, 40, 5);
  Frames[toolbar->frameId].marginLeft = 10;
  Frames[toolbar->frameId].alignment =
      ALIGNMENT_CENTER_VERTICAL | ALIGNMENT_CENTER_HORIZONTAL;
  addFrameToVStack(drawPage, toolbar->frameId);

  createButton(0, 0, 40, 40, "", COLOR_TRANSPARENT, (Color){255, 60, 60, false},
               COLOR_WHITE, font_12, onRedClick, NULL);
  Frames[frameCount - 1].scrollType = 0;
  addFrameToHStack(toolbar, frameCount - 1);

  createButton(0, 0, 40, 40, "", COLOR_TRANSPARENT, COLOR_CYAN, COLOR_WHITE,
               font_12, onCyanClick, NULL);
  Frames[frameCount - 1].scrollType = 0;
  addFrameToHStack(toolbar, frameCount - 1);

  createButton(0, 0, 40, 40, "Erase", COLOR_WHITE, (Color){60, 60, 60, false},
               COLOR_WHITE, font_12, onEraserClick, NULL);
  Frames[frameCount - 1].scrollType = 0;
  addFrameToHStack(toolbar, frameCount - 1);

  createButton(0, 0, 40, 40, "Clr", COLOR_BLACK, COLOR_WHITE, COLOR_GRAY,
               font_12, onClearClick, NULL);
  Frames[frameCount - 1].scrollType = 0;
  addFrameToHStack(toolbar, frameCount - 1);

  demoCanvas = createCanvas(0, 0, SCREEN_WIDTH - 40, 200);
  Frames[demoCanvas->frameId].marginLeft = 20;
  addFrameToVStack(drawPage, demoCanvas->frameId);

  // Initialize Global Toast Manager
  initToastManager(font_12);

  // Main interaction loop
  while (true) {
    updateWindowManager();
    if (!updateScreen())
      break;
  }

  return 0;
}
