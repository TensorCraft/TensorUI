#include "../../TensorUI/Animation/Tween.h"
#include "../../TensorUI/TensorUI.h"
#include "../../TensorUI/Theme/Theme.h"
#include "../../hal/screen/screen.h"
#include "../../hal/stdio/stdio.h"
#include "../../hal/time/time.h"
#include "Apps/Calculator/Calculator.h"
#include "Apps/Bounce/Bounce.h"
#include "Apps/Snake/Snake.h"
#include "../../TensorUI/Image/Image.h"
#include "icon_about.h"
#include "icon_calc.h"
#include "icon_paint.h"
#include "icon_settings.h"
#include "icon_snake.h"
#include "icon_bounce.h"
#include "icon_edit.h"
#include "logo_md.h"

extern Frame Frames[];
extern int frameCount;
extern bool renderFlag;

Font font_22;
Font font_18;
Font font_12;

int currentTab = 0;
int themeSelection = 0; // 0 for Dark, 1 for Light
HStack *homePage = NULL;
float homeSavedScrollX = 0.0f;
// globalFAB = NULL; // Removed FAB

VStack *settingsPage = NULL;
VStack *aboutPage = NULL;
VStack *drawPage = NULL;
VStack *materialPage = NULL;
VStack *editorPage = NULL;
VStack *editorEditPage = NULL;
VStack *renderLabPage = NULL;

int homeFrames[50];
int homeCount = 0;
int settingsFrames[50];
int settingsCount = 0;
int aboutFrames[50];
int aboutCount = 0;
int drawFrames[50];
int drawCount = 0;

Canvas *demoCanvas = NULL;
TextField *editorHomeField = NULL;
TextField *editorEditField = NULL;
Keyboard *editorKeyboard = NULL;
char editorBuffer[128] = "Hello from TensorUI";
VStack *demoAppPage = NULL;
VStack *demoSubPage1 = NULL;
VStack *demoSubPage2 = NULL;
VStack *homePage3 = NULL;
Label *renderLabCounterLabel = NULL;
Label *renderLabModeLabel = NULL;
ProgressBar *renderLabPrimaryBar = NULL;
ProgressBar *renderLabMirrorBar = NULL;
Button *renderLabToggleButton = NULL;

typedef struct {
  int frameId;
  int width;
  int height;
  int markerX;
  int markerWidth;
  int direction;
  bool running;
} RenderTrack;

RenderTrack renderTrack = {0};
int renderLabAtomicStep = 0;
bool isRoundScreen =
#ifdef ROUND_SCREEN
    true;
#else
    false;
#endif

static ScreenInsets appSafeInsets(void) {
  return getScreenSafeInsets();
}

static int appContentWidth(void) {
  return getScreenContentWidth(appSafeInsets());
}

static int appCardWidth(void) {
  return appContentWidth();
}

static int appInnerCardWidth(void) {
  int w = appCardWidth() - 24;
  return w > 1 ? w : 1;
}

static int appFieldWidth(void) {
  return appContentWidth();
}

static int appFieldLeft(void) {
  return 0;
}

static int appTitleLeft(void) {
  return 0;
}

static void setFrameSwipePassthrough(int frameId) {
  Frames[frameId].scrollType = 0;
}

static void configureAppPageFrame(int frameId, Color bgColor, bool isApp) {
  ScreenInsets insets = appSafeInsets();
  applyFramePaddingInsets(frameId, insets);
  Frames[frameId].bgcolor = bgColor;
  configureFrameAsAppSurface(frameId, isApp);
}

// Forward declarations for Demo App
void onLaunchDemoApp(void *arg);
void onLaunchRenderLab(void *arg);
void goToSubPage1(void *arg);
void goToSubPage2(void *arg);

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
    setCanvasColor(demoCanvas, M3_PRIMARY);
  showToast("Color: Primary", 1000);
}

void onEraserClick(void *self) {
  if (demoCanvas)
    setCanvasColor(demoCanvas, demoCanvas->backgroundColor);
  showToast("Eraser Selected", 1000);
}

void onCircleStamp(void *arg) {
  if (demoCanvas)
    drawCircleOnCanvas(demoCanvas, 100, 100, 30, M3_PRIMARY, true);
}
void onRectStamp(void *arg) {
  if (demoCanvas)
    drawRectOnCanvas(demoCanvas, 50, 50, 40, 40, M3_SECONDARY, false);
}

// PAGE SETUP FUNCTIONS
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

void onCheckToggle(bool checked) {
  showToast(checked ? "Checked!" : "Unchecked!", 1000);
}

void onRadioSelect(void *arg) { showToast((char *)arg, 1000); }

void onShowDialog(void *arg) {
  Dialog *d = createDialog("System Alert", "This is a Material Design dialog.",
                           "OK", "CANCEL", font_18, font_12, NULL, NULL, NULL);
  showDialog(d);
}

bool onPaintCloseRequest(void *self) {
  showToast("Paint window close requested", 1000);
  return true;
}

// App Launch Handlers
void initSettingsPage();
void initAboutPage();
void initDrawPage();
void initMaterialPage();
void initEditorPage();
void initRenderLabPage();
void onEditorFieldClick(void *arg);
static void onEditorResume(void *self);
static void onSystemNotificationOpenRenderLab(void *arg);
static void onEditorKeyboardDone(void *arg);

void onLaunchSettings(void *self) {
  initSettingsPage();
  if (settingsPage) {
    pushWindow(settingsPage->frameId);
  }
  showToast("Launching Settings", 800);
}

void onLaunchAbout(void *self) {
  initAboutPage();
  if (aboutPage) pushWindow(aboutPage->frameId);
}

void onLaunchDraw(void *arg) {
  initDrawPage();
  if (drawPage) pushWindow(drawPage->frameId);
}

void onLaunchMaterial(void *arg) {
  initMaterialPage();
  if (materialPage) pushWindow(materialPage->frameId);
}

void onLaunchSnake(void *arg) { pushSnakeApp(); }
void onLaunchBounce(void *arg) { pushBounceApp(); }
void onLaunchEditor(void *arg) {
  initEditorPage();
  if (editorPage) pushWindow(editorPage->frameId);
}

void onLaunchRenderLab(void *arg) {
  initRenderLabPage();
  if (renderLabPage) pushWindow(renderLabPage->frameId);
}


static void onSettingsPageDestroy(void *self) { vstackOnDestroy(self); settingsPage = NULL; }
static void onAboutPageDestroy(void *self) { vstackOnDestroy(self); aboutPage = NULL; }
static void onMaterialPageDestroy(void *self) { vstackOnDestroy(self); materialPage = NULL; }
static void onDrawPageDestroy(void *self) { vstackOnDestroy(self); drawPage = NULL; demoCanvas = NULL; }
static void onEditorPageDestroy(void *self) { vstackOnDestroy(self); editorPage = NULL; editorHomeField = NULL; }
static void onEditorEditPageDestroy(void *self) { vstackOnDestroy(self); editorEditPage = NULL; editorEditField = NULL; editorKeyboard = NULL; }
static void onDemoAppDestroy(void *self) { vstackOnDestroy(self); demoAppPage = NULL; }
static void onSubPage1Destroy(void *self) { vstackOnDestroy(self); demoSubPage1 = NULL; }
static void onSubPage2Destroy(void *self) { vstackOnDestroy(self); demoSubPage2 = NULL; }
static void onHomePage3Destroy(void *self) { vstackOnDestroy(self); homePage3 = NULL; }
static void onHomePageDestroy(void *self) { hstackOnDestroy(self); homePage = NULL; }
static void onRenderLabDestroy(void *self) {
  vstackOnDestroy(self);
  renderLabPage = NULL;
  renderLabCounterLabel = NULL;
  renderLabModeLabel = NULL;
  renderLabPrimaryBar = NULL;
  renderLabMirrorBar = NULL;
  renderLabToggleButton = NULL;
  renderTrack = (RenderTrack){0};
  renderLabAtomicStep = 0;
}

static Color renderTrackGetPixel(void *self, int x, int y) {
  RenderTrack *track = (RenderTrack *)self;
  Color bg = (Color){28, 28, 30, false};
  Color stripe = (Color){44, 44, 48, false};
  Color accent = M3_PRIMARY;
  if (x >= track->markerX && x < track->markerX + track->markerWidth) {
    return accent;
  }
  if (((x / 12) + (y / 10)) % 2 == 0) {
    return stripe;
  }
  return bg;
}

static void renderTrackUpdate(void *self) {
  RenderTrack *track = (RenderTrack *)self;
  if (!track->running) return;

  int previousX = track->markerX;
  track->markerX += track->direction * 3;
  if (track->markerX <= 0) {
    track->markerX = 0;
    track->direction = 1;
  }
  if (track->markerX + track->markerWidth >= track->width) {
    track->markerX = track->width - track->markerWidth;
    track->direction = -1;
  }

  invalidateFrameRect(track->frameId, previousX, 0, track->markerWidth, track->height);
  invalidateFrameRect(track->frameId, track->markerX, 0, track->markerWidth, track->height);
}

static void updateRenderLabReadout(const char *modeText) {
  char counterText[64];
  hal_snprintf(counterText, sizeof(counterText), "Atomic commits: %d", renderLabAtomicStep);
  if (renderLabCounterLabel) updateLabel(renderLabCounterLabel, counterText);
  if (renderLabModeLabel && modeText) updateLabel(renderLabModeLabel, modeText);
}

static void onRenderLabAtomicPulse(void *arg) {
  renderLabAtomicStep++;
  float progress = (float)((renderLabAtomicStep * 17) % 100) / 100.0f;
  float mirror = 1.0f - (progress * 0.75f);
  if (mirror < 0.08f) mirror = 0.08f;

  beginAtomicRenderUpdate();
  if (renderLabPrimaryBar) setProgress(renderLabPrimaryBar, progress);
  if (renderLabMirrorBar) setProgress(renderLabMirrorBar, mirror);
  updateRenderLabReadout("Mode: atomic batch updated");
  endAtomicRenderUpdate();
}

static void onRenderLabToggleTrack(void *arg) {
  renderTrack.running = !renderTrack.running;
  if (renderTrack.frameId >= 0 && renderTrack.frameId < MAX_FRAMES) {
    Frames[renderTrack.frameId].continuousRender = renderTrack.running;
    invalidateFrame(renderTrack.frameId);
  }
  if (renderLabToggleButton) {
    updateButtonText(renderLabToggleButton, renderTrack.running ? "Pause Local Refresh" : "Resume Local Refresh");
  }
  updateRenderLabReadout(renderTrack.running ? "Mode: local dirty rect sweep" : "Mode: paused");
}

static void onRenderLabReset(void *arg) {
  renderLabAtomicStep = 0;
  renderTrack.markerX = 0;
  renderTrack.direction = 1;
  beginAtomicRenderUpdate();
  if (renderLabPrimaryBar) setProgress(renderLabPrimaryBar, 0.12f);
  if (renderLabMirrorBar) setProgress(renderLabMirrorBar, 0.88f);
  updateRenderLabReadout("Mode: reset to baseline");
  endAtomicRenderUpdate();
  invalidateFrame(renderTrack.frameId);
}

void initSettingsPage() {
  if (settingsPage) return;
  int contentW = appContentWidth();
  int fieldLeft = appFieldLeft();
  settingsPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 15);
  setVStackScrollbar(settingsPage, true);
  configureAppPageFrame(settingsPage->frameId, M3_DARK_BG, true);
  Frames[settingsPage->frameId].onDestroy = onSettingsPageDestroy;

  Label *lDisplay = createLabel(0, 0, 0, 0, "Display Settings", M3_PRIMARY, COLOR_TRANSPARENT, font_18);
  Frames[lDisplay->frameId].marginLeft = appTitleLeft();
  addFrameToVStack(settingsPage, lDisplay->frameId);

  Label *lBrightness = createLabel(0, 0, 0, 0, "Brightness", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  Frames[lBrightness->frameId].marginLeft = appTitleLeft();
  addFrameToVStack(settingsPage, lBrightness->frameId);

  Slider *slBrightness = createSlider(0, 0, contentW, 30, 0.75f, onSliderChange);
  Frames[slBrightness->frameId].marginLeft = fieldLeft;
  addFrameToVStack(settingsPage, slBrightness->frameId);

  Label *lFeatures = createLabel(0, 0, 0, 0, "Features", M3_PRIMARY, COLOR_TRANSPARENT, font_18);
  Frames[lFeatures->frameId].marginLeft = appTitleLeft();
  Frames[lFeatures->frameId].marginTop = 20;
  addFrameToVStack(settingsPage, lFeatures->frameId);

  char *settingNames[] = {"Wi-Fi", "Bluetooth", "Do Not Disturb", "Location", "Auto-Rotate", "Dark Mode", "Night Shift"};
  for (int i = 0; i < 7; i++) {
    HStack *row = createHStack(0, 0, contentW, 40, 0);
    Frames[row->frameId].marginLeft = fieldLeft;
    Frames[row->frameId].alignment = ALIGNMENT_CENTER_VERTICAL | ALIGNMENT_RIGHT;
    addFrameToVStack(settingsPage, row->frameId);

    Label *lRow = createLabel(0, 0, contentW - 70, 30, settingNames[i], M3_ON_SURFACE, COLOR_TRANSPARENT, font_18);
    addFrameToHStack(row, lRow->frameId);

    Toggle *tRow = createToggle(0, 0, i % 2 == 0, NULL);
    addFrameToHStack(row, tRow->frameId);
  }

  HStack *aaRow = createHStack(0, 0, contentW, 40, 0);
  Frames[aaRow->frameId].marginLeft = fieldLeft;
  Frames[aaRow->frameId].alignment = ALIGNMENT_CENTER_VERTICAL | ALIGNMENT_RIGHT;
  addFrameToVStack(settingsPage, aaRow->frameId);

  Label *lAA = createLabel(0, 0, contentW - 70, 30, "Anti-Aliasing", M3_SECONDARY, COLOR_TRANSPARENT, font_18);
  addFrameToHStack(aaRow, lAA->frameId);

  Toggle *tAA = createToggle(0, 0, false, onAAToggleWrapper);
  addFrameToHStack(aaRow, tAA->frameId);
}

void initAboutPage() {
  if (aboutPage) return;
  int contentW = appContentWidth();
  int fieldLeft = appFieldLeft();
  aboutPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 15);
  setVStackScrollbar(aboutPage, true);
  configureAppPageFrame(aboutPage->frameId, M3_DARK_BG, true);
  Frames[aboutPage->frameId].onDestroy = onAboutPageDestroy;

  Label *lInfo = createLabel(0, 0, 0, 0, "System Info", M3_PRIMARY, COLOR_TRANSPARENT, font_18);
  Frames[lInfo->frameId].marginLeft = appTitleLeft();
  addFrameToVStack(aboutPage, lInfo->frameId);

  Label *lVersion = createLabel(0, 0, 0, 0, "TensorUI 1.2 Enterprise", M3_ON_SURFACE, COLOR_TRANSPARENT, font_12);
  Frames[lVersion->frameId].marginLeft = appTitleLeft();
  addFrameToVStack(aboutPage, lVersion->frameId);

  Label *lMem = createLabel(0, 0, 0, 0, "Memory Usage", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  Frames[lMem->frameId].marginLeft = appTitleLeft();
  Frames[lMem->frameId].marginTop = 20;
  addFrameToVStack(aboutPage, lMem->frameId);

  ProgressBar *pbMem = createProgressBar(0, 0, contentW, 6, 0.42f);
  Frames[pbMem->frameId].marginLeft = fieldLeft;
  addFrameToVStack(aboutPage, pbMem->frameId);

  Label *lStorage = createLabel(0, 0, 0, 0, "Storage Usage", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  Frames[lStorage->frameId].marginLeft = appTitleLeft();
  Frames[lStorage->frameId].marginTop = 15;
  addFrameToVStack(aboutPage, lStorage->frameId);

  ProgressBar *pbStorage = createProgressBar(0, 0, contentW, 6, 0.88f);
  Frames[pbStorage->frameId].marginLeft = fieldLeft;
  addFrameToVStack(aboutPage, pbStorage->frameId);

  Button *btnToast = createButton(0, 0, contentW, 44, "Test Toast Queue", M3_ON_SURFACE, M3_SURFACE, M3_SURFACE_VARIANT, font_12, onQueueToastClick, NULL);
  Frames[btnToast->frameId].marginLeft = fieldLeft;
  Frames[btnToast->frameId].marginTop = 20;
  addFrameToVStack(aboutPage, btnToast->frameId);
}

void initDrawPage() {
  if (drawPage) return;
  int cardW = appCardWidth();
  int innerCardW = appInnerCardWidth();
  drawPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 8);
  configureAppPageFrame(drawPage->frameId, M3_DARK_BG, true);
  Frames[drawPage->frameId].onCloseRequest = onPaintCloseRequest;
  Frames[drawPage->frameId].onDestroy = onDrawPageDestroy;

  Label *lPaint = createLabel(0, 0, 0, 0, "Paint", M3_PRIMARY, COLOR_TRANSPARENT, font_18);
  Frames[lPaint->frameId].marginLeft = appTitleLeft();
  addFrameToVStack(drawPage, lPaint->frameId);

  Label *lTools = createLabel(0, 0, 0, 0, "Tools", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  Frames[lTools->frameId].marginLeft = appTitleLeft();
  addFrameToVStack(drawPage, lTools->frameId);

  Card *toolCard = createCard(0, 0, cardW, 76, M3_SURFACE);
  Frames[toolCard->frameId].marginLeft = appFieldLeft();
  Frames[toolCard->frameId].marginBottom = 12;
  addFrameToVStack(drawPage, toolCard->frameId);

  HStack *toolbar = createHStack(0, 0, innerCardW, 52, 6);
  setHStackScrollbar(toolbar, false);
  Frames[toolbar->frameId].alignment = ALIGNMENT_CENTER_VERTICAL | ALIGNMENT_CENTER_HORIZONTAL;
  addFrameToCard(toolCard, toolbar->frameId);

  Button* btnRed = createButton(0, 0, 36, 36, "", COLOR_TRANSPARENT, (Color){255, 60, 60, false}, COLOR_WHITE, font_12, onRedClick, NULL);
  setButtonCornerRadius(btnRed, 18);
  addFrameToHStack(toolbar, btnRed->frameId);

  Button* btnCyan = createButton(0, 0, 36, 36, "", COLOR_TRANSPARENT, COLOR_CYAN, COLOR_WHITE, font_12, onCyanClick, NULL);
  setButtonCornerRadius(btnCyan, 18);
  addFrameToHStack(toolbar, btnCyan->frameId);

  Button* btnEraser = createButton(0, 0, 52, 34, "Eraser", COLOR_WHITE, (Color){60, 60, 60, false}, COLOR_WHITE, font_12, onEraserClick, NULL);
  setButtonCornerRadius(btnEraser, 8);
  addFrameToHStack(toolbar, btnEraser->frameId);

  Button* btnClear = createButton(0, 0, 52, 34, "Clear", M3_ON_SURFACE, M3_SURFACE_VARIANT, M3_OUTLINE, font_12, onClearClick, NULL);
  setButtonCornerRadius(btnClear, 8);
  addFrameToHStack(toolbar, btnClear->frameId);

  Button* btnO = createButton(0, 0, 34, 34, "O", M3_ON_PRIMARY, M3_PRIMARY, M3_PRIMARY, font_12, onCircleStamp, NULL);
  setButtonCornerRadius(btnO, 17);
  addFrameToHStack(toolbar, btnO->frameId);

  Button* btnSquare = createButton(0, 0, 34, 34, "[]", M3_ON_PRIMARY, M3_SECONDARY, M3_SECONDARY, font_12, onRectStamp, NULL);
  setButtonCornerRadius(btnSquare, 4);
  addFrameToHStack(toolbar, btnSquare->frameId);

  Label *lCanvas = createLabel(0, 0, 0, 0, "Canvas", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  Frames[lCanvas->frameId].marginLeft = appTitleLeft();
  Frames[lCanvas->frameId].marginTop = 4;
  addFrameToVStack(drawPage, lCanvas->frameId);

  Card *canvasCard = createCard(0, 0, cardW, 160, (Color){25, 25, 25, false});
  setCardContentInsets(canvasCard, 12, 12, 12, 12);
  Frames[canvasCard->frameId].marginLeft = appFieldLeft();
  addFrameToVStack(drawPage, canvasCard->frameId);

  demoCanvas = createCanvas(0, 0, innerCardW, 136);
  addFrameToCard(canvasCard, demoCanvas->frameId);
}

void initMaterialPage() {
  if (materialPage) return;
  int cardW = appCardWidth();
  int innerCardW = appInnerCardWidth();
  materialPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10);
  configureAppPageFrame(materialPage->frameId, M3_DARK_BG, true);
  Frames[materialPage->frameId].onDestroy = onMaterialPageDestroy;

  Label *lTitle = createLabel(0, 0, 0, 0, "Material 3", M3_PRIMARY, COLOR_TRANSPARENT, font_18);
  Frames[lTitle->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[lTitle->frameId].marginBottom = 2;
  addFrameToVStack(materialPage, lTitle->frameId);

  Label *lSub = createLabel(0, 0, 0, 0, "Color & motion for touch.", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  Frames[lSub->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[lSub->frameId].marginBottom = 6;
  addFrameToVStack(materialPage, lSub->frameId);

  Card *c1 = createCard(0, 0, cardW, 114, M3_SURFACE);
  Frames[c1->frameId].marginLeft = appFieldLeft();
  Frames[c1->frameId].marginBottom = 15;
  addFrameToVStack(materialPage, c1->frameId);

  HStack *row1 = createHStack(0, 0, innerCardW, 28, 6);
  addFrameToCard(c1, row1->frameId);
  Label *lNotif = createLabel(0, 0, 0, 0, "Enable Notifications", M3_ON_SURFACE, COLOR_TRANSPARENT, font_12);
  addFrameToHStack(row1, lNotif->frameId);
  CheckBox *cb1 = createCheckBox(0, 0, true, onCheckToggle);
  addFrameToHStack(row1, cb1->frameId);

  HStack *row2 = createHStack(0, 0, innerCardW, 28, 6);
  addFrameToCard(c1, row2->frameId);
  Label *lTheme = createLabel(0, 0, 0, 0, "Theme:", M3_ON_SURFACE, COLOR_TRANSPARENT, font_12);
  addFrameToHStack(row2, lTheme->frameId);
  RadioButton *rb1 = createRadioButton(0, 0, &themeSelection, 0, onRadioSelect, "Dark Mode");
  addFrameToHStack(row2, rb1->frameId);
  Label *lDark = createLabel(0, 0, 0, 0, "Dark", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  addFrameToHStack(row2, lDark->frameId);
  RadioButton *rb2 = createRadioButton(0, 0, &themeSelection, 1, onRadioSelect, "Light Mode");
  addFrameToHStack(row2, rb2->frameId);
  Label *lLight = createLabel(0, 0, 0, 0, "Light", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  addFrameToHStack(row2, lLight->frameId);

  Label *lSpacer = createLabel(0, 0, 10, 10, "", COLOR_TRANSPARENT, COLOR_TRANSPARENT, font_12);
  addFrameToVStack(materialPage, lSpacer->frameId);

  ListTile *lt1 = createListTile(0, 0, cardW, "System", "Software is up to date", font_18, font_12, onShowDialog, NULL);
  Frames[lt1->frameId].marginLeft = appFieldLeft();
  Frames[lt1->frameId].marginBottom = 15;
  addFrameToVStack(materialPage, lt1->frameId);

  Card *c2 = createCard(0, 0, cardW, 64, M3_SURFACE);
  Frames[c2->frameId].marginLeft = appFieldLeft();
  Frames[c2->frameId].marginBottom = 15;
  addFrameToVStack(materialPage, c2->frameId);

  HStack *switchRow = createHStack(0, 0, innerCardW, 40, 6);
  Label *lAdapt = createLabel(0, 0, 140, 28, "Adaptive Brightness", M3_ON_SURFACE, COLOR_TRANSPARENT, font_12);
  addFrameToHStack(switchRow, lAdapt->frameId);
  Switch *sw1 = createSwitch(0, 0, true, NULL);
  addFrameToHStack(switchRow, sw1->frameId);
  addFrameToCard(c2, switchRow->frameId);

  Card *c3 = createCard(0, 0, cardW, 64, M3_SURFACE);
  Frames[c3->frameId].marginLeft = appFieldLeft();
  addFrameToVStack(materialPage, c3->frameId);

  HStack *actionRow = createHStack(0, 0, innerCardW, 40, 8);
  Frames[actionRow->frameId].alignment = ALIGNMENT_CENTER_VERTICAL | ALIGNMENT_CENTER_HORIZONTAL;
  addFrameToCard(c3, actionRow->frameId);

  Button* btnP = createButton(0, 0, 90, 32, "Primary", M3_ON_PRIMARY, M3_PRIMARY, (Color){180, 160, 240, false}, font_12, NULL, NULL);
  setButtonCornerRadius(btnP, 16);
  addFrameToHStack(actionRow, btnP->frameId);
  
  Button* btnS = createButton(0, 0, 90, 32, "Secondary", M3_ON_SURFACE, M3_SURFACE_VARIANT, M3_OUTLINE, font_12, NULL, NULL);
  setButtonCornerRadius(btnS, 16);
  addFrameToHStack(actionRow, btnS->frameId);
}

void initEditorPage() {
  if (editorPage) return;
  editorPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10);
  configureAppPageFrame(editorPage->frameId, M3_DARK_BG, false);

  Label *lNotes = createLabel(0, 0, 0, 0, "Notes", M3_PRIMARY, COLOR_TRANSPARENT, font_18);
  Frames[lNotes->frameId].marginLeft = appTitleLeft();
  addFrameToVStack(editorPage, lNotes->frameId);

  editorHomeField = createTextField(0, 0, appFieldWidth(), 44, editorBuffer, font_12, M3_ON_SURFACE, M3_SURFACE, onEditorFieldClick, NULL);
  int tfid = getTextFieldFrameId(editorHomeField);
  Frames[tfid].marginLeft = appFieldLeft();
  Frames[tfid].marginTop = 8;
  addFrameToVStack(editorPage, tfid);
  Frames[editorPage->frameId].onResume = onEditorResume;
  Frames[editorPage->frameId].onDestroy = onEditorPageDestroy;
}

void onEditorFieldClick(void *arg) {
  if (!editorEditPage) {
    ScreenInsets insets = appSafeInsets();
    int contentW = getScreenContentWidth(insets);
    int contentH = getScreenContentHeight(insets);
    editorEditPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 6);
    configureAppPageFrame(editorEditPage->frameId, M3_DARK_BG, false);

    editorEditField = createTextField(0, 0, contentW, contentH / 2 - 18,
                                      editorBuffer, font_12, M3_ON_SURFACE, M3_SURFACE, NULL, NULL);
    Frames[getTextFieldFrameId(editorEditField)].marginLeft = 0;
    Frames[getTextFieldFrameId(editorEditField)].marginTop = 6;
    setTextFieldExternalBuffer(editorEditField, editorBuffer, 128);
    setTextFieldEditingEnabled(editorEditField, true);
    setTextFieldMultiline(editorEditField, true);
    setTextFieldPlaceholder(editorEditField, "Notes");
    addFrameToVStack(editorEditPage, getTextFieldFrameId(editorEditField));

    editorKeyboard = createKeyboard(0, 0, contentW, contentH / 2 - 8,
                                    isRoundScreen, editorBuffer, 128, font_12);
    setKeyboardDoneAction(editorKeyboard, onEditorKeyboardDone, NULL);
    Frames[getKeyboardFrameId(editorKeyboard)].marginLeft = 0;
    addFrameToVStack(editorEditPage, getKeyboardFrameId(editorKeyboard));
    
    Frames[editorEditPage->frameId].onDestroy = onEditorEditPageDestroy;
  }
  pushWindow(editorEditPage->frameId);
}

static void onEditorKeyboardDone(void *arg) {
  clearTextInputTarget();
  clearFocusedFrame();
  if (WinMgr.count > 0) {
    popWindow();
    return;
  }

  if (editorEditPage && Frames[editorEditPage->frameId].enabled) {
    destroyFrame(editorEditPage->frameId);
    invalidateFullScreen();
  }
  if (editorHomeField) {
    updateTextField(editorHomeField, editorBuffer);
  }
}

static void onEditorResume(void *self) {
  updateTextField(editorHomeField, editorBuffer);
}

static void onSystemNotificationOpenRenderLab(void *arg) {
  (void)arg;
  onLaunchRenderLab(NULL);
}

void initRenderLabPage() {
  if (renderLabPage) return;

  int cardW = appCardWidth();
  int innerCardW = appInnerCardWidth();
  renderLabPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10);
  configureAppPageFrame(renderLabPage->frameId, M3_DARK_BG, true);
  Frames[renderLabPage->frameId].onDestroy = onRenderLabDestroy;

  Label *title = createLabel(0, 0, 0, 0, "Render Lab", M3_PRIMARY, COLOR_TRANSPARENT, font_18);
  Frames[title->frameId].marginLeft = appTitleLeft();
  addFrameToVStack(renderLabPage, title->frameId);

  Label *subtitle = createLabel(0, 0, innerCardW, 34, "Round-screen demo for partial refresh and atomic UI commits.", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  subtitle->isMultiLine = true;
  subtitle->wrapWidth = innerCardW;
  subtitle->dirty = true;
  Frames[subtitle->frameId].marginLeft = appFieldLeft();
  addFrameToVStack(renderLabPage, subtitle->frameId);

  Card *statusCard = createCard(0, 0, cardW, 112, M3_SURFACE);
  setCardContentInsets(statusCard, 12, 12, 12, 12);
  Frames[statusCard->frameId].marginLeft = appFieldLeft();
  addFrameToVStack(renderLabPage, statusCard->frameId);

  VStack *statusStack = createVStack(0, 0, innerCardW, 88, 8);
  addFrameToCard(statusCard, statusStack->frameId);

  renderLabCounterLabel = createLabel(0, 0, 0, 0, "Atomic commits: 0", M3_ON_SURFACE, COLOR_TRANSPARENT, font_18);
  addFrameToVStack(statusStack, renderLabCounterLabel->frameId);

  renderLabModeLabel = createLabel(0, 0, innerCardW, 20, "Mode: local dirty rect sweep", M3_OUTLINE, COLOR_TRANSPARENT, font_12);
  addFrameToVStack(statusStack, renderLabModeLabel->frameId);

  renderLabPrimaryBar = createProgressBar(0, 0, innerCardW, 8, 0.12f);
  renderLabPrimaryBar->barColor = M3_PRIMARY;
  renderLabPrimaryBar->bgColor = M3_SURFACE_VARIANT;
  addFrameToVStack(statusStack, renderLabPrimaryBar->frameId);

  renderLabMirrorBar = createProgressBar(0, 0, innerCardW, 8, 0.88f);
  renderLabMirrorBar->barColor = M3_SECONDARY;
  renderLabMirrorBar->bgColor = M3_SURFACE_VARIANT;
  addFrameToVStack(statusStack, renderLabMirrorBar->frameId);

  Card *trackCard = createCard(0, 0, cardW, 88, M3_SURFACE);
  setCardContentInsets(trackCard, 12, 12, 12, 12);
  Frames[trackCard->frameId].marginLeft = appFieldLeft();
  addFrameToVStack(renderLabPage, trackCard->frameId);

  Label *trackLabel = createLabel(0, 0, 0, 0, "Dirty-rect sweep", M3_ON_SURFACE, COLOR_TRANSPARENT, font_12);
  addFrameToCard(trackCard, trackLabel->frameId);

  renderTrack.width = innerCardW;
  renderTrack.height = 40;
  renderTrack.markerWidth = 26;
  renderTrack.markerX = 0;
  renderTrack.direction = 1;
  renderTrack.running = true;
  renderTrack.frameId = requestFrame(renderTrack.width, renderTrack.height, 0, 0, &renderTrack, NULL, renderTrackGetPixel, NULL, NULL);
  Frames[renderTrack.frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[renderTrack.frameId].marginTop = 8;
  Frames[renderTrack.frameId].onUpdate = renderTrackUpdate;
  Frames[renderTrack.frameId].continuousRender = true;
  addFrameToCard(trackCard, renderTrack.frameId);

  Button *atomicButton = createButton(0, 0, cardW, 40, "Atomic Step", M3_ON_PRIMARY, M3_PRIMARY, M3_PRIMARY, font_12, onRenderLabAtomicPulse, NULL);
  setButtonCornerRadius(atomicButton, 18);
  Frames[atomicButton->frameId].marginLeft = appFieldLeft();
  Frames[atomicButton->frameId].marginTop = 4;
  addFrameToVStack(renderLabPage, atomicButton->frameId);

  renderLabToggleButton = createButton(0, 0, cardW, 40, "Pause Local Refresh", M3_ON_SURFACE, M3_SURFACE, M3_OUTLINE, font_12, onRenderLabToggleTrack, NULL);
  setButtonCornerRadius(renderLabToggleButton, 18);
  Frames[renderLabToggleButton->frameId].marginLeft = appFieldLeft();
  addFrameToVStack(renderLabPage, renderLabToggleButton->frameId);

  Button *resetButton = createButton(0, 0, cardW, 38, "Reset Demo", M3_ON_SURFACE, M3_SURFACE_VARIANT, M3_OUTLINE, font_12, onRenderLabReset, NULL);
  setButtonCornerRadius(resetButton, 18);
  Frames[resetButton->frameId].marginLeft = appFieldLeft();
  addFrameToVStack(renderLabPage, resetButton->frameId);
}

static void onHomePause(void *self) {
  HStack *hs = (HStack *)self;
  if (hs) {
    cancelHStackInteraction(hs);
    homeSavedScrollX = hs->scrollX;
  }
}

static void onHomeResume(void *self) {
  HStack *hs = (HStack *)self;
  if (hs) {
    hs->scrollX = homeSavedScrollX;
  }
}

// End of app cleaning hooks

void onLaunchDemoApp(void *arg) {
  if (!demoAppPage) {
    demoAppPage = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 20);
    configureAppPageFrame(demoAppPage->frameId, M3_DARK_BG, true);
    Frames[demoAppPage->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
    Frames[demoAppPage->frameId].onDestroy = onDemoAppDestroy;
    
    Label *lDemo = createLabel(0,0,0,0, "Demo App Root", M3_PRIMARY, COLOR_TRANSPARENT, font_18);
    addFrameToVStack(demoAppPage, lDemo->frameId);
    
    Button *b1 = createButton(0, 0, 160, 40, "Go to Page 1", M3_ON_SURFACE, M3_SURFACE, M3_OUTLINE, font_12, goToSubPage1, NULL);
    addFrameToVStack(demoAppPage, b1->frameId);
    
    Button *b2 = createButton(0, 0, 160, 40, "Go to Page 2", M3_ON_SURFACE, M3_SURFACE, M3_OUTLINE, font_12, goToSubPage2, NULL);
    addFrameToVStack(demoAppPage, b2->frameId);
  }
  pushWindow(demoAppPage->frameId);
}

void goToSubPage1(void *arg) {
  if (!demoSubPage1) {
    demoSubPage1 = createVStack(0,0, SCREEN_WIDTH, SCREEN_HEIGHT, 20);
    configureAppPageFrame(demoSubPage1->frameId, M3_PRIMARY, false);
    Frames[demoSubPage1->frameId].onDestroy = onSubPage1Destroy;
    Label *lSub1 = createLabel(0,0,0,0, "Sub Page 1", M3_ON_PRIMARY, COLOR_TRANSPARENT, font_18);
    Frames[lSub1->frameId].marginLeft = appTitleLeft();
    addFrameToVStack(demoSubPage1, lSub1->frameId);
  }
  pushWindow(demoSubPage1->frameId);
}

void goToSubPage2(void *arg) {
  if (!demoSubPage2) {
    demoSubPage2 = createVStack(0,0, SCREEN_WIDTH, SCREEN_HEIGHT, 20);
    configureAppPageFrame(demoSubPage2->frameId, M3_SECONDARY, false);
    Frames[demoSubPage2->frameId].onDestroy = onSubPage2Destroy;
    Label *lSub2 = createLabel(0,0,0,0, "Sub Page 2", M3_ON_SURFACE, COLOR_TRANSPARENT, font_18);
    Frames[lSub2->frameId].marginLeft = appTitleLeft();
    addFrameToVStack(demoSubPage2, lSub2->frameId);
  }
  pushWindow(demoSubPage2->frameId);
}

void onLaunchCalc(void *arg) { pushCalculatorApp(); }

void addAppIcon(HStack *container, const Color *bitmap, const char *name, void (*onClick)(void*)) {
    VStack *iconGroup = createVStack(0, 0, 90, 110, 8); // Larger for 2x2
    Frames[iconGroup->frameId].bgcolor = COLOR_TRANSPARENT;
    Frames[iconGroup->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
    setFrameSwipePassthrough(iconGroup->frameId);
    
    // Icon

    UIImage *img = createUIImageFromBitmap(0, 0, 64, 64, bitmap, 64, 64);
    setUIImageBorderRadius(img, 16);
    Frames[img->frameId].onClick = onClick;
    Frames[img->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
    addFrameToVStack(iconGroup, img->frameId);
    
    // Label
    Label *lbl = createLabel(0, 0, 0, 0, name, COLOR_WHITE, COLOR_TRANSPARENT, font_12);
    lbl->alignment = ALIGNMENT_CENTER_HORIZONTAL;
    addFrameToVStack(iconGroup, lbl->frameId);
    
    addFrameToHStack(container, iconGroup->frameId);
}

// Helper to create a page
VStack *createGridPage(HStack *parent) {
  VStack *page = createVStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 15);
  configureAppPageFrame(page->frameId, COLOR_TRANSPARENT, false);
  Frames[page->frameId].bgcolor = COLOR_TRANSPARENT;
  setFrameSwipePassthrough(page->frameId);
  addFrameToHStack(parent, page->frameId);
  return page;
}


// End of implementation sections

int main() {
  init_screen("TensorOS Premium", SCREEN_WIDTH, SCREEN_HEIGHT);

  font_22 = loadFont("./fonts/LiberationSans-Regular22.bfont");
  font_18 = loadFont("./fonts/LiberationSans-Regular18.bfont");
  font_12 = loadFont("./fonts/LiberationSans-Regular12.bfont");

  // ==========================================
  // BOOT SEQUENCE
  // ==========================================
  initDefaultTheme();
  initTweenEngine();

  HStack *logoStack =
      createHStack(0, SCREEN_HEIGHT / 2 - 40, SCREEN_WIDTH, 40, 0);

  Frames[logoStack->frameId].alignment =
      ALIGNMENT_CENTER_HORIZONTAL | ALIGNMENT_CENTER_VERTICAL;

  Label *lb1 = createLabel(0, 0, 0, 0, "Tensor", M3_PRIMARY, COLOR_TRANSPARENT, font_22);
  addFrameToHStack(logoStack, lb1->frameId);

  Label *lb2 = createLabel(0, 0, 0, 0, "OS", M3_ON_SURFACE, COLOR_TRANSPARENT, font_22);
  addFrameToHStack(logoStack, lb2->frameId);

  ProgressBar *bootProgress = createProgressBar(
      SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 + 30, 160, 4, 0.0f);
  bootProgress->barColor = M3_PRIMARY;
  bootProgress->bgColor = M3_SURFACE_VARIANT;

  long long bootStart = current_timestamp_ms();
  while (true) {
    long long elapsed = current_timestamp_ms() - bootStart;
    float p = elapsed / 2500.0f; // 2.5 seconds boot
    p = p * p;                   // non-linear progression
    if (p > 1.0f)
      p = 1.0f;

    setProgress(bootProgress, p);
    if (!updateScreen())
      return 0;

    if (p >= 1.0f) {
      hal_sleep_ms(500); // pause at 100%
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
  // HOME SCREEN (4x4 Grid with Paging)
  // ==========================================
  homePage = createHStack(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
  setHStackPaging(homePage, true);
  Frames[homePage->frameId].bgcolor = (Color){20, 20, 20, false};
  Frames[homePage->frameId].onPause = onHomePause;
  Frames[homePage->frameId].onResume = onHomeResume;
  Frames[homePage->frameId].onDestroy = onHomePageDestroy;
  configureFrameAsAppSurface(homePage->frameId, true); // Root level app
  pushWindow(homePage->frameId);

  VStack* page1 = createGridPage(homePage);
  configureAppPageFrame(page1->frameId, COLOR_TRANSPARENT, false);
  
  // Row 1
  ScreenInsets homeInsets = appSafeInsets();
  int rowW = getScreenContentWidth(homeInsets);
  HStack* r1 = createHStack(0, 0, rowW, 100, 20);
  Frames[r1->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[r1->frameId].marginLeft = 0;
  setFrameSwipePassthrough(r1->frameId);
  addFrameToVStack(page1, r1->frameId);
  addAppIcon(r1, icon_calc_data, "Calculator", onLaunchCalc);
  addAppIcon(r1, icon_snake_data, "Snake", onLaunchSnake);

  // Row 2
  HStack* r2 = createHStack(0, 0, rowW, 100, 20);
  Frames[r2->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[r2->frameId].marginLeft = 0;
  setFrameSwipePassthrough(r2->frameId);
  addFrameToVStack(page1, r2->frameId);
  addAppIcon(r2, icon_settings_data, "Settings", onLaunchSettings);
  addAppIcon(r2, icon_paint_data, "Paint", onLaunchDraw);

  VStack* page2 = createGridPage(homePage);
  configureAppPageFrame(page2->frameId, COLOR_TRANSPARENT, false);

  // Row 3
  HStack* r3 = createHStack(0, 0, rowW, 100, 20);
  Frames[r3->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[r3->frameId].marginLeft = 0;
  setFrameSwipePassthrough(r3->frameId);
  addFrameToVStack(page2, r3->frameId);
  addAppIcon(r3, icon_about_data, "System", onLaunchAbout);
  addAppIcon(r3, logo_md_data, "Material", onLaunchMaterial);

  // Row 4
  HStack* r4 = createHStack(0, 0, rowW, 100, 20);
  Frames[r4->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[r4->frameId].marginLeft = 0;
  setFrameSwipePassthrough(r4->frameId);
  addFrameToVStack(page2, r4->frameId);
  addAppIcon(r4, icon_bounce_data, "Bounce", onLaunchBounce);
  addAppIcon(r4, icon_edit_data, "Editor", onLaunchEditor);

  homePage3 = createGridPage(homePage);
  configureAppPageFrame(homePage3->frameId, COLOR_TRANSPARENT, false);
  Frames[homePage3->frameId].onDestroy = onHomePage3Destroy;

  HStack* r5 = createHStack(0, 0, rowW, 100, 20);
  Frames[r5->frameId].alignment = ALIGNMENT_CENTER_HORIZONTAL;
  Frames[r5->frameId].marginLeft = 0;
  setFrameSwipePassthrough(r5->frameId);
  addFrameToVStack(homePage3, r5->frameId);
  addAppIcon(r5, logo_md_data, "PagingApp", onLaunchDemoApp);
  addAppIcon(r5, icon_settings_data, "RenderLab", onLaunchRenderLab);


  // Apps will be initialized lazily on launch.

  // Initialize Global Toast Manager
  initToastManager(font_12);
  initSystemUI(font_12, font_12);
  setStatusBarTitle("TensorOS");
  pushSystemNotification("System ready", "Drag down from the top edge to open notifications...");
  pushSystemNotificationWithAction("Render Lab", "Atomic updates and partial refresh are available in Render Lab...",
                                   onSystemNotificationOpenRenderLab, NULL);

  // Main interaction loop
  while (true) {
    updateWindowManager();
    updateSystemUI();
    updateTweens(); // Process animations

    if (!updateScreen())
      break;
  }

  return 0;
}
