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
TAB_DEMO := examples/tab_demo/tab_demo

.PHONY: all clean
all: $(TARGET) $(TAB_DEMO)

CORE_SRCS := src/ui_primitives.c src/ui_widget.c src/ui_container.c src/ui_column.c src/ui_row.c src/ui_button.c src/ui_appbar.c src/ui_checkbox.c src/ui_progressring.c src/ui_progressbar.c src/ui_shadow.c src/ui_slider.c src/ui_switch.c src/ui_radio.c src/ui_scene.c src/ui_text.c src/ui_tab.c src/ui_system_styles.c src/ui_font.c src/ui_font_lores.c src/hal/hal_test_sdl.c

# Build demos
$(TARGET): $(CORE_SRCS) tests/main.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $^ -o $@ $(LDFLAGS) $(SDL_LDFLAGS)

$(TAB_DEMO): $(CORE_SRCS) examples/tab_demo/main.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $^ -o $@ $(LDFLAGS) $(SDL_LDFLAGS)

clean:
	rm -f $(TARGET)
