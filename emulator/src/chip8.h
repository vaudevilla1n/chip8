#pragma once

#include <stdint.h>

#define CHIP8_MEMORY		0x1000
#define CHIP8_RESERVED_MEMORY	0x200
#define CHIP8_AVAILABLE_MEMORY	0xe00

#define CHIP8_PROGRAM_START	0x200

#define CHIP8_STACK_SIZE	0x10
#define CHIP8_STACK_ADDR	0xEFF

#define CHIP8_TIMER_FREQ	60

#define CHIP8_FONT_ADDR		0x000
#define CHIP8_FONT_SIZE		5
#define CHIP8_TOTAL_FONTS	16

#define CHIP8_DISPLAY_HEIGHT	32
#define CHIP8_DISPLAY_WIDTH	64

#define CHIP8_DISPLAY_X		((COLS - CHIP8_DISPLAY_WIDTH) / 2)
#define CHIP8_DISPLAY_Y		((LINES - CHIP8_DISPLAY_HEIGHT) / 2)

extern uint8_t	chip8_fonts[CHIP8_TOTAL_FONTS][CHIP8_FONT_SIZE];

enum chip8_register {
	REG_V0, REG_V1, REG_V2, REG_V3, REG_V4, REG_V5, REG_V6,
	REG_V7, REG_V8, REG_V9, REG_VA, REG_VB, REG_VC, REG_VD,
	REG_VE, REG_VF, CHIP8_REGISTERS,
};

#define CHIP8_ERROR_MAX	128

enum chip8_state {
	CHIP8_OK,
	CHIP8_ERR,
	CHIP8_EXIT,
	CHIP8_COMPLETE,
};

struct chip8 {
	uint8_t delay_timer;
	uint8_t sound_timer;

	uint16_t pc;

	uint16_t i_reg;

	uint8_t v_reg[CHIP8_REGISTERS];

	uint8_t sp;
	uint16_t stack[CHIP8_STACK_SIZE];

	uint8_t memory[CHIP8_MEMORY];

	uint8_t display[CHIP8_DISPLAY_HEIGHT][CHIP8_DISPLAY_WIDTH];

	enum chip8_state state;
	char err[CHIP8_ERROR_MAX];
};

void chip8_init(struct chip8 *machine, const char *rom_path);

void chip8_dump_registers(const struct chip8 *machine);
void chip8_diagnostics(const struct chip8 *machine);

void chip8_run(struct chip8 *machine);
