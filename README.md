# BareUI Framework

Лёгкий, модульный UI-движок на **C99** для 320×240 экранов с возможностью портовки на *ESP32/FreeRTOS*. Все графические данные пишутся в RGB565-фреймбуфер, а HAL-интерфейс изолирует остальной код от железа.

- `include/ui_primitives.h` и `src/ui_primitives.c` — потокобезопасный контекст, framebuffer, очереди событий (сенсор, клавиатура), рисование прямоугольников и текста через шрифт BareUI, API управления шрифтами и событиями.
- `include/ui_widget.h` и `src/ui_widget.c` — начальная абстракция виджетов: иерархия, bounds, отрисовка, маршрутизация событий и стилизации.
- `include/ui_container.h` и `src/ui_container.c` — контейнеры с layout-режимами (вертикальный, горизонтальный, overlay), spacing и стилизацией, чтобы упорядочивать дочерние виджеты.
- `include/ui_column.h` и `src/ui_column.c` — специализированный Column-контрол с вертикальным размещением, spacing, расширением дочерних элементов, прокруткой и RTL/Wrap-настройками.
- `include/ui_row.h` и `src/ui_row.c` — Row-эквивалент с горизонтальным урегулированием, прокруткой, RTL и wrap-поддержкой.
- `include/ui_button.h` и `src/ui_button.c` — текстовая кнопка с обработкой касаний/клавиш, hover/focus/long-press-callbacks, собственным стилем границы и тенями.
- `include/ui_shadow.h` и `src/ui_shadow.c` — вспомогательный рендер тени прямоугольных областей для виджетов.
- `include/ui_text.h` и `src/ui_text.c` — базовый текстовый виджет с цветом, фоновой заливкой, выравниванием, обрезкой/сворачиванием строк и настройками переноса.
- `include/ui_scene.h` и `src/ui_scene.c` — менеджер сцены, который содержит HAL/фреймбуфер, владеет корнем виджетов, маршалит события, вызывает пользовательские tick-хуки и управляет главным циклом. `include/ui_core.h` теперь включает этот слой как публичный вход в стек.
- `include/ui_font.h` + `src/ui_font.c` — шаблонный растровый шрифт, поддерживающий ASCII и кириллицу, механизмы поиска глифа и выставления интервала.
- `src/font/bareui_font_data.h` — данные шрифта, генерируемые из векторного TTF с помощью `tools/build_font.py`.
- `include/ui_hal_test.h` + `src/hal/hal_test_sdl.c` — десктопный HAL с 4× масштабированием framebuffer-а и эмуляцией тачскрина/клавиатуры через SDL2.
- `tests/main.c` — новая демонстрационная сцена widgets: колонка, строки, текстовые блоки и кнопки, стилизованные через `ui_style_t` с on-click и clock-tick логикой.

## System styles

`include/ui_system_styles.h` теперь открывает четыре палитро-ориентированных системных стиля: `UI_SYSTEM_STYLE_AMETHYST` (Pale purple, Medium slate blue, Rebecca purple, Davy's gray, Dark purple), `UI_SYSTEM_STYLE_VERDANT` (Battleship gray, Ash gray, Tea green, Cream, Flax), `UI_SYSTEM_STYLE_DAWN` (Raisin black, Slate gray, Isabelline, Bone, Khaki) и `UI_SYSTEM_STYLE_CLASSIC` (Classic Noir — белый, черный и серые градации). Каждое состояние заполняет `ui_style_t` (фон, текст, акцент, рамка и тень) через `ui_system_style_fill`, а `ui_system_style_name` возвращает человекочитаемое название.

Обновлённая демо-сцена `tests/main.c` теперь состоит из заголовка, блока статуса и единственной кнопки. Кнопка выводит имя текущего стиля в stdout, показывает его в UI и переключается на следующую палитру при каждом нажатии. Оболочка, заголовок и статус используют палитру Seashell / Champagne pink / Rosy brown / Bone / Redwood, чтобы все цвета пользователя также появились на экране.

Outlined buttons are medium-emphasis controls for actions that are important but not the primary call to action. They rely on a thin border and transparent background so they can sit next to filled buttons as secondary alternatives; the demo in `tests/main.c` shows how to style them.

## Button styles
`ui_button` может имитировать разные стили, но для повышения читаемости важно выбирать их правильно. Первичная кнопка — это заполненный фон (filled tonal) без лишних теней, а elevated-кнопка просто добавляет к ней аккуратную тень. Чтобы тень не "ползла" по интерфейсу, применяйте такие кнопки только тогда, когда элемент должен визуально отделиться от текстурированного или узорного фона. To prevent shadow creep, only use them when absolutely necessary, such as when the button requires visual separation from a patterned background.

Демонстрация в `tests/main.c` выделяет отдельный блок с паттерном и подписью, чтобы показать, как elevated-кнопка выглядит поверх узорного фона и почему тень стоит использовать лишь в таком сценарии.

Filled buttons have the most visual impact after the FloatingActionButton and should be reserved for important, final actions such as Save, Join now, or Confirm. The `tests/main.c` showcase now adds a dedicated filled row with bold backgrounds, hover/pressed feedback, and a lift shadow to illustrate this pattern.
Use `ui_button_set_filled_style` to bootstrap that look on any button without recreating the style from scratch.

## Шрифтовая цепочка
1. Откройте `tools/build_font.py`, укажите нужный TTF (`FONT_PATH`) и запустите `python tools/build_font.py > src/font/bareui_font_data.h` (нужен Python3 и библиотека Pillow).
2. Векторный файл содержимого генерирует колонны 8×8 для каждого символа, включая `Ё/ё` и `А-Я/а-я`. Не забудьте перегенерировать шрифт после замены TTF или добавления символов.
3. Рендер текста декодирует UTF‑8 и использует текущий `bareui_font_t`. Расстояние между символами задаётся `BAREUI_FONT_SPACING` (по умолчанию 1 колонка) — увеличьте до 2, если на вашей матрице «слипается».

## Тестирование на компьютере
Нужен SDL2 (`libsdl2-dev` на Debian/Ubuntu). Затем:
```bash
make
./tests/main
./examples/tab_demo/tab_demo
```
Окно 1280×960 (масштаб 4×) показывает framebuffer 320×240, мышь эмулирует сенсор, `q` закрывает. Русский текст демонстрирует поддержку кириллицы.
