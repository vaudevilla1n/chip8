#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define unused(x)	(void)(x)
#define todo(x)		do { fprintf(stderr, "todo: \'%s\'\n", (x)); abort(); } while (0)

#define countof(arr)	(sizeof(arr) / sizeof((arr)[0]))

#define CHIP8_MEMORY		0x1000
#define CHIP8_DATA_REGISTERS	0x10
#define CHIP8_STACK_MEMORY	0x1000

#define CHIP8_CLOCK_TICKS	60

#define CHIP8_DISPLAY_HEIGHT	32
#define CHIP8_DISPLAY_WIDTH	64

enum chip8_register {
	V0, V1, V2, V3, V4, V5, V6,
	V7, V8, V9, VA, VB, VC, VD,
	VE, VF
};

struct chip8 {
	uint8_t delay_timer;
	uint8_t sound_timer;

	uint16_t pc;

	uint16_t i_reg : 12;

	uint8_t reg[CHIP8_DATA_REGISTERS];

	uint8_t stack[CHIP8_STACK_MEMORY];
	uint8_t memory[CHIP8_MEMORY];

	uint8_t display[CHIP8_DISPLAY_HEIGHT][CHIP8_DISPLAY_WIDTH];
} virt_machine;

void chip8_init(struct chip8 *machine);
void chip8_diagnostics(const struct chip8 *machine);

int main(void)
{
	chip8_init(&virt_machine);
	chip8_diagnostics(&virt_machine);
}

static void clear_display(struct chip8 *machine)
{
	for (size_t y = 0; y < CHIP8_DISPLAY_HEIGHT; y++)
		for (size_t x = 0; x < CHIP8_DISPLAY_WIDTH; x++)
			machine->display[y][x] = 0;
}

static void subroutine_return(struct chip8 *machine)
{
	unused(machine);
	todo("subroutine_return");
}

static void subroutine_enter(struct chip8 *machine, const uint16_t addr)
{
	unused(addr);
	unused(machine);
	todo("subroutine_enter");
}

[[noreturn]] static void invalid_opcode(const struct chip8 *machine, const uint16_t op)
{
	fprintf(stderr, "unknown opcode (%hx) @ (0x%hx) aborting...\n", machine->pc, op);
	abort();
}

static void instruction(struct chip8 *machine)
{
	const uint16_t op = *(uint16_t *)(machine->memory + machine->pc);

	const uint8_t op_code_id = (op >> 12);
	switch (op_code_id) {
	case 0x0: {
		switch (op) {
		case 0x00E0:	clear_display(machine); return;
		case 0x00EE:	subroutine_return(machine); return;

		default:	invalid_opcode(machine, op);
		}
	} break;

	case 0x1:	machine->pc = (op & 0x0FFF); return;

	case 0x2:	subroutine_enter(machine, (op & 0x0FFF)); return;

	case 0x3: {
		const uint8_t val = (op & 0xFF);
		const enum chip8_register reg = ((op >> 8) & 0xF);

		if (machine->reg[reg] == val)
			machine->pc += 2;
	} break;

	case 0x4: {
		const uint8_t val = (op & 0xFF);
		const enum chip8_register reg = ((op >> 8) & 0xF);

		if (machine->reg[reg] != val)
			machine->pc += 2;
	} break;

	case 0x5: {
		if ((op & 0xF) != 0)
			invalid_opcode(machine, op);

		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		if (machine->reg[reg0] == machine->reg[reg1])
			machine->pc += 2;
	} break;

	case 0x6: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		machine->reg[reg] = (op & 0xFF);
	} break;

	case 0x7: {
	} break;

	case 0x8: {
	} break;

	case 0x9: {
	} break;

	case 0xa: {
	} break;

	case 0xb: {
	} break;

	case 0xc: {
	} break;

	case 0xd: {
	} break;

	case 0xe: {
	} break;

	case 0xf: {
	} break;
	}

	machine->pc += 2;
}

void chip8_init(struct chip8 *machine)
{
	*machine = (struct chip8){ 0 };

	machine->delay_timer = CHIP8_CLOCK_TICKS;
	machine->sound_timer = CHIP8_CLOCK_TICKS;
}

void chip8_diagnostics(const struct chip8 *machine)
{
	printf("(chip8)\n");
	printf("total data registers: %zu\n", sizeof(machine->reg));
	printf("total available memory: %zu\n", sizeof(machine->memory));
	printf("total stack memory: %zu\n", sizeof(machine->stack));
	printf("display (%zu x %zu)\n", sizeof(machine->display[0]), countof(machine->display));
}

