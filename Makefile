CC = gcc
CFLAGS = -Wall -I./TensorUI/Font -I./hal/screen -I./hal/time -I./TensorUI/Color \
         -ITensorUI/Label -ITensorUI/Button -ITensorUI/SwitchTab \
         -ITensorUI/ProgressBar -ITensorUI/VStack -ITensorUI/Toast \
         -ITensorUI/Canvas -ITensorUI -Ihal/font -Ihal/input -Ihal/mem -Ihal/str -Ihal/rand \
         -ITensorUI/CheckBox -ITensorUI/RadioButton -ITensorUI/Card -ITensorUI/ListTile -ITensorUI/Dialog -ITensorUI/Switch \
         -ITensorUI/Animation -ITensorUI/Theme -ITensorUI/Image -ITensorUI/Keyboard -ITensorUI/TextField \
         -ITensorUI/SystemUI \
         -Iexamples/TensorOS_Demo/Apps/Snake -Iexamples/TensorOS_Demo/Apps/Calculator -Iexamples/TensorOS_Demo/Apps/Bounce -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lSDL2

SRCFILES = examples/TensorOS_Demo/main.c \
           hal/screen/screen.c \
           hal/time/time.c \
           hal/input/input_hal.c \
           hal/mem/mem.c \
           hal/str/str.c \
           hal/stdio/stdio.c \
           hal/math/math.c \
           hal/rand/rand.c \
           TensorUI/Color/color.c \
           TensorUI/Font/font.c \
           hal/font/font_hal.c \
           TensorUI/Label/Label.c \
           TensorUI/Button/Button.c \
           TensorUI/Button/FAB.c \
           TensorUI/SwitchTab/Toggle.c \
           TensorUI/SwitchTab/SwitchTab.c \
           TensorUI/ProgressBar/ProgressBar.c \
           TensorUI/ProgressBar/Slider.c \
           TensorUI/VStack/VStack.c \
           TensorUI/Toast/Toast.c \
           TensorUI/Canvas/Canvas.c \
           TensorUI/CheckBox/CheckBox.c \
           TensorUI/RadioButton/RadioButton.c \
           TensorUI/Card/Card.c \
           TensorUI/ListTile/ListTile.c \
           TensorUI/Dialog/Dialog.c \
           TensorUI/Switch/Switch.c \
           TensorUI/Animation/Tween.c \
           TensorUI/Theme/Theme.c \
           TensorUI/Image/Image.c \
           TensorUI/Keyboard/Keyboard.c \
           TensorUI/TextField/TextField.c \
           TensorUI/SystemUI/SystemUI.c \
           examples/TensorOS_Demo/Apps/Snake/Snake.c \
           examples/TensorOS_Demo/Apps/Calculator/Calculator.c \
           examples/TensorOS_Demo/Apps/Bounce/Bounce.c \
           TensorUI/WindowManager.c \
           TensorUI/TensorUI.c


TARGET = sdl_example
TEST_TARGET = render_invalidation_test
TEST_SRCFILES = tests/render_invalidation_test.c \
                hal/screen/screen.c \
                hal/mem/mem.c \
                TensorUI/Color/color.c

all: $(TARGET)

$(TARGET): $(SRCFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCFILES) $(LDFLAGS)

$(TEST_TARGET): $(TEST_SRCFILES)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCFILES) $(LDFLAGS)

check: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)
	@echo "Build verification passed: $(TARGET) + $(TEST_TARGET)"

clean:
	rm -f $(TARGET) $(TEST_TARGET)

run: all
	./$(TARGET)

.PHONY: all check clean run
