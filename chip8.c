#include <time.h>
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
	REG_V0, REG_V1, REG_V2, REG_V3, REG_V4, REG_V5, REG_V6,
	REG_V7, REG_V8, REG_V9, REG_VA, REG_VB, REG_VC, REG_VD,
	REG_VE, REG_VF
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

static void draw(const uint8_t x, const uint8_t y, const uint8_t height)
{
	unused(x);
	unused(y);
	unused(height);
	todo("draw");
}

static uint16_t sprite_address(const uint8_t sprite_id)
{
	unused(sprite_id);
	todo("sprite_address");
}

static uint8_t key_pressed(void)
{
	todo("key_pressed");
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

static void store_binary_coded_decimal(struct chip8 *machine, const uint8_t val)
{
	unused(val);
	unused(machine);
	todo("store_binary_coded_decimal");
}

static void write_registers(struct chip8 *machine, const enum chip8_register reg)
{
	unused(reg);
	unused(machine);
	todo("write_registers");
}

static void read_registers(struct chip8 *machine, const enum chip8_register reg)
{
	unused(reg);
	unused(machine);
	todo("read_registers");
}


[[noreturn]] static void invalid_opcode(const struct chip8 *machine, const uint16_t op)
{
	fprintf(stderr, "unknown opcode (%hx) @ (0x%hx) aborting...\n", machine->pc - 2, op);
	abort();
}

/*
	prolly gonna add some bounds check 
*/
static void advance_program_counter(struct chip8 *machine)
{
	machine->pc += 2;
}

static void instruction(struct chip8 *machine)
{
	const uint16_t op = *(uint16_t *)(machine->memory + machine->pc);
	advance_program_counter(machine);

	const uint8_t op_code_id = (op >> 12);
	switch (op_code_id) {
	case 0x0: {
		switch (op) {
		case 0x00E0:	clear_display(machine); return;
		case 0x00EE:	subroutine_return(machine); return;

		default:	invalid_opcode(machine, op);
		}
	} break;

	case 0x1: {
		const uint16_t addr = (op & 0x0FFF);

		machine->pc = addr;
	} break;

	case 0x2: {
		const uint16_t addr = (op & 0x0FFF);

		subroutine_enter(machine, addr);
	} break;

	case 0x3: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		if (machine->reg[reg] == val)
			advance_program_counter(machine);
	} break;

	case 0x4: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		if (machine->reg[reg] != val)
			advance_program_counter(machine);
	} break;

	case 0x5: {
		if ((op & 0xF) != 0)
			invalid_opcode(machine, op);

		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		if (machine->reg[reg0] == machine->reg[reg1])
			advance_program_counter(machine);
	} break;

	case 0x6: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		machine->reg[reg] = val;
	} break;

	case 0x7: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		machine->reg[reg] += val;
	} break;

	case 0x8: {
		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		switch (op & 0xF) {
		case 0:		machine->reg[reg0] = machine->reg[reg1]; break;

		case 1:		machine->reg[reg0] |= machine->reg[reg1]; break;

		case 2:		machine->reg[reg0] &= machine->reg[reg1]; break;

		case 3:		machine->reg[reg0] ^= machine->reg[reg1]; break;

		case 4:		machine->reg[reg0] += machine->reg[reg1]; break;

		case 5:		machine->reg[reg0] -= machine->reg[reg1]; break;

		case 6:		machine->reg[reg0] >>= machine->reg[reg1]; break;

		case 7:		machine->reg[reg0] = machine->reg[reg1] - machine->reg[reg0]; break;

		case 0xE:	machine->reg[reg0] <<= machine->reg[reg1]; break;

		default:	invalid_opcode(machine, op);
		}
	} break;

	case 0x9: {
		if ((op & 0xF) != 0)
			invalid_opcode(machine, op);

		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		if (machine->reg[reg0] != machine->reg[reg1])
			advance_program_counter(machine);
	} break;

	case 0xA: {
		const uint16_t addr = (op & 0x0FFF);

		machine->i_reg = addr;
	} break;

	case 0xB: {
		const uint16_t addr = (op & 0x0FFF);

		machine->pc = machine->reg[REG_V0] + addr;
	} break;

	case 0xC: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		machine->reg[reg] = (rand() & 0xFF) & val;
	} break;

	case 0xD: {
		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		const uint8_t sprite_height = (op & 0xF);

		draw(machine->reg[reg0], machine->reg[reg1], sprite_height);
	} break;

	case 0xE: {
		const uint8_t code = (op & 0xFF);
		const enum chip8_register reg = ((op >> 8) & 0xF);

		switch (code) {
		case 0x9E: {
			if (key_pressed() == machine->reg[reg])
				advance_program_counter(machine);
		} break;

		case 0xA1: {
			if (key_pressed() != machine->reg[reg])
				advance_program_counter(machine);
		} break;

		default:	invalid_opcode(machine, op);
		}
	} break;

	case 0xF: {
		const uint8_t code = (op & 0xFF);
		const enum chip8_register reg = ((op >> 8) & 0xF);

		switch (code) {
		case 0x07:	machine->reg[reg] = machine->delay_timer; break;

		case 0x0A:	machine->reg[reg] = key_pressed(); break;

		case 0x15:	machine->delay_timer = machine->reg[reg]; break;

		case 0x18:	machine->sound_timer = machine->reg[reg]; break;
		
		case 0x1E:	machine->i_reg += machine->reg[reg]; break;

		case 0x29:	machine->i_reg = sprite_address(machine->reg[reg]); break;

		case 0x33:	store_binary_coded_decimal(machine, machine->reg[reg]); break;

		case 0x55:	write_registers(machine, reg); break;

		case 0x65:	read_registers(machine, reg); break;

		default:	invalid_opcode(machine, op);
		}
	} break;
	}
}

void chip8_init(struct chip8 *machine)
{
	srand(time(nullptr));

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

