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
$(TARGET): src/ui_primitives.c src/ui_widget.c src/ui_container.c src/ui_column.c src/ui_row.c src/ui_button.c src/ui_checkbox.c src/ui_progressring.c src/ui_progressbar.c src/ui_shadow.c src/ui_switch.c src/ui_scene.c src/ui_text.c src/ui_font.c src/ui_font_lores.c src/hal/hal_test_sdl.c tests/main.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $^ -o $@ $(LDFLAGS) $(SDL_LDFLAGS)

clean:
	rm -f $(TARGET)
