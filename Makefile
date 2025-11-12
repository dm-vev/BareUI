CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Iinclude -O2
LDFLAGS := -pthread -lm

SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LDFLAGS := $(shell sdl2-config --libs 2>/dev/null)

ifeq ($(SDL_CFLAGS),)
SDL_CFLAGS := -I/usr/include/SDL2
endif

ifeq ($(SDL_LDFLAGS),)
SDL_LDFLAGS := -lSDL2
endif

TARGET := tests/main

.PHONY: all clean
all: $(TARGET)

	# Build demo
$(TARGET): src/ui_core.c src/ui_font.c src/ui_font_lores.c src/hal/hal_test_sdl.c tests/main.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $^ -o $@ $(LDFLAGS) $(SDL_LDFLAGS)

clean:
	rm -f $(TARGET)
