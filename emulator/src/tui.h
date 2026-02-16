#pragma once

#include "chip8.h"

#include <curses.h>

void tui_init(void);
void tui_draw_screen_size(void);
void tui_draw_display_border(void);
void tui_draw_display(const struct chip8 *machine);
void tui_draw_register_info(const struct chip8 *machine);
void tui_deinit(void);

#define tui_info(fmt, ...)	mvprintw(LINES - 2, 0, fmt __VA_OPT__(,)__VA_ARGS__)
