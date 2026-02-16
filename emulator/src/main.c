/*
	basic chip8 implementation

	all thanks to:
		cowgod @ http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#Dxyn
		wikipedia @ https://en.wikipedia.org/wiki/CHIP-8
*/
#include "tui.h"
#include "chip8.h"
#include "common.h"

#include <stdio.h>

static struct chip8 emulator;

[[noreturn]] void usage(void)
{
	die("usage: chip8 ROM");
}

int main(int argc, char **argv)
{
	if (argc != 2)
		usage();
	
	const char *rom_path = argv[1];
	chip8_init(&emulator, rom_path);

	tui_init();

	while (getch() != 'q') {
		tui_draw_display_border();

		switch (emulator.state) {
		case CHIP8_OK: {
			chip8_run(&emulator);
			tui_draw_display(&emulator);
			tui_draw_register_info(&emulator);
		} break;

		case CHIP8_ERR: {
			tui_info("%s: exiting...", emulator.err);
			emulator.state = CHIP8_EXIT;
		} break;

		case CHIP8_COMPLETE: {
			tui_info("program completed");
			emulator.state = CHIP8_EXIT;
		} break;

		case CHIP8_EXIT:
			break;

		default:	__builtin_unreachable();
		}

		tui_draw_screen_size();
	}

	tui_deinit();
}

