#include "tui.h"

#include "common.h"

void assert_screen_size(void)
{
	if (CHIP8_DISPLAY_HEIGHT + 2 > LINES || CHIP8_DISPLAY_WIDTH + 2 > COLS) {
		tui_deinit();
		die("screen too small to display emulator");
	}
}

#define	BLACK	1
#define	WHITE	2

void tui_init(void)
{
	initscr();

	assert_screen_size();

	start_color();

	init_pair(BLACK, COLOR_BLACK, COLOR_BLACK);
	init_pair(WHITE, COLOR_WHITE, COLOR_WHITE);

	cbreak();
	noecho();
	nodelay(stdscr, true);

	curs_set(0);
}

static void draw_box(const int y, const int x, const int h, const int w)
{
	mvhline(y, x, ACS_ULCORNER, 1);
	mvhline(y + h, x, ACS_LLCORNER, 1);
	mvhline(y + h, x + w, ACS_LRCORNER, 1);
	mvhline(y, x + w, ACS_URCORNER, 1);

	mvhline(y, x + 1, 0, w - 1);
	mvvline(y + 1, x, 0, h - 1);
	mvhline(y + h, x + 1, 0, w - 1);
	mvvline(y + 1, x + w, 0, h - 1);
}

void tui_draw_screen_size(void)
{
	if (LINES - 1 > CHIP8_DISPLAY_Y)
		mvprintw(LINES - 1, 0, "%d x %d screen", COLS, LINES);
}

void tui_draw_display_border(void)
{
	draw_box(CHIP8_DISPLAY_Y - 1, CHIP8_DISPLAY_X - 1, CHIP8_DISPLAY_HEIGHT + 2, CHIP8_DISPLAY_WIDTH + 2);
}

void tui_draw_display(const struct chip8 *machine)
{
	for (int y = 0; y < CHIP8_DISPLAY_HEIGHT; y++) {
		move(CHIP8_DISPLAY_Y + y, CHIP8_DISPLAY_X);
		for (int x = 0; x < CHIP8_DISPLAY_WIDTH; x++) {
			const uint8_t p = machine->display[y][x];
			attr_on(COLOR_PAIR(p ? WHITE : BLACK), nullptr);
			addch(ACS_BLOCK);
		}
	}

	standend();
}

#define REGISTERS_PER_COL	4

static bool can_draw_register_info(void)
{
	/* 
		9 characters
		'XX: 0x00 ' * 4

		(3 special + 16 data registers) / 4 regs per column
		= 5 columns
	*/
	const int x_needed = 36;
	const int y_needed = 5;

	return (x_needed < COLS && y_needed < CHIP8_DISPLAY_Y);
}

void tui_draw_register_info(const struct chip8 *machine)
{
	if (!can_draw_register_info())
		return;
	
	move(0, 0);
	
	printw("PC: 0x%.3hx SP: 0x%.3hx I: 0x%.3hx\n", machine->pc, machine->sp, machine->i_reg);

	for (enum chip8_register reg = REG_V0; reg < CHIP8_REGISTERS; reg++)
		printw("V%X: 0x%.2hhx%c", reg, machine->v_reg[reg], ((reg + 1) % REGISTERS_PER_COL) ? ' ' : '\n'); 
}

void tui_deinit(void)
{
	endwin();
}
