CC = gcc
CFLAGS = -Wall -I./TensorUI/Font -I./hal/screen -I./hal/time -I./TensorUI/Color \
         -ITensorUI/Label -ITensorUI/Button -ITensorUI/SwitchTab \
         -ITensorUI/ProgressBar -ITensorUI/VStack -ITensorUI/Toast \
         -ITensorUI/Canvas -ITensorUI -Ihal/font -Ihal/input \
         -Iexamples/TensorOS_Demo/Apps/Snake -Iexamples/TensorOS_Demo/Apps/Calculator -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lSDL2

SRCFILES = examples/TensorOS_Demo/main.c \
           hal/screen/screen.c \
           hal/time/time.c \
           hal/input/input_hal.c \
           TensorUI/Color/color.c \
           TensorUI/Font/font.c \
           hal/font/font_hal.c \
           TensorUI/Label/Label.c \
           TensorUI/Button/Button.c \
           TensorUI/SwitchTab/Toggle.c \
           TensorUI/SwitchTab/SwitchTab.c \
           TensorUI/ProgressBar/ProgressBar.c \
           TensorUI/ProgressBar/Slider.c \
           TensorUI/VStack/VStack.c \
           TensorUI/Toast/Toast.c \
           TensorUI/Canvas/Canvas.c \
           examples/TensorOS_Demo/Apps/Snake/Snake.c \
           examples/TensorOS_Demo/Apps/Calculator/Calculator.c \
           TensorUI/WindowManager.c

TARGET = sdl_example

all: $(TARGET)

$(TARGET): $(SRCFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCFILES) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
