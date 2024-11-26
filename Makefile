# This Makefile is only tested on MacOS 15.0.1

CC = gcc
CFLAGS = -Wall -Icolor -Ifont -Ihal/screen -ITensorUI/Label -ITensorUI/TextEdit -L/opt/homebrew/lib -lSDL2 -I/opt/homebrew/include
LDFLAGS = 

BUILDDIR = build

SRCFILES = main.c \
			color/color.c \
			font/font.c \
			hal/screen/screen.c \
			TensorUI/Label/Label.c \

OBJFILES = $(patsubst %.c, $(BUILDDIR)/%.o, $(SRCFILES))

TARGET = $(BUILDDIR)/program

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

debug:
	@echo "Source files: $(SRCFILES)"
	@echo "Object files: $(OBJFILES)"
	@echo "Target: $(TARGET)"

.PHONY: all clean debug
