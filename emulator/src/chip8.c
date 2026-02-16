#define _DEFAULT_SOURCE

#include "chip8.h"

#include "common.h"

#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

uint8_t	chip8_fonts[CHIP8_TOTAL_FONTS][CHIP8_FONT_SIZE] = {
	[0x0] 	= { 0xF0, 0x90, 0x90, 0x90, 0xF0 },
	[0x1] 	= { 0x20, 0x60, 0x20, 0x20, 0x70 },
	[0x2] 	= { 0xF0, 0x10, 0xF0, 0x80, 0xF0 },
	[0x3] 	= { 0xF0, 0x10, 0xF0, 0x10, 0xF0 },
	[0x4] 	= { 0x90, 0x90, 0xF0, 0x10, 0x10 },
	[0x5] 	= { 0xF0, 0x80, 0xF0, 0x10, 0xF0 },
	[0x6] 	= { 0xF0, 0x10, 0x20, 0x40, 0x40 },
	[0x7] 	= { 0xF0, 0x90, 0xF0, 0x90, 0xF0 },
	[0x8] 	= { 0xF0, 0x90, 0xF0, 0x90, 0xF0 },
	[0x9] 	= { 0xF0, 0x90, 0xF0, 0x10, 0xF0 },
	[0xA] 	= { 0xF0, 0x90, 0xF0, 0x90, 0x90 },
	[0xB] 	= { 0xE0, 0x90, 0xE0, 0x90, 0xE0 },
	[0xC] 	= { 0xF0, 0x80, 0x80, 0x80, 0xF0 },
	[0xD] 	= { 0xE0, 0x90, 0x90, 0x90, 0xE0 },
	[0xE] 	= { 0xF0, 0x80, 0xF0, 0x80, 0xF0 },
	[0xF] 	= { 0xF0, 0x80, 0xF0, 0x80, 0x80 },
};


static void load_rom(struct chip8 *machine, const char *path)
{
	struct stat stats;
	if (stat(path, &stats))
		die("unable to get rom's (\'%s\') size: %s", path, strerror(errno));
	
	if (stats.st_size > CHIP8_AVAILABLE_MEMORY)
		die("rom (\'%s\') is too large", path);

	const uint16_t rom_size = stats.st_size; 
	
	FILE *f = fopen(path, "r");
	if (!f)
		die("unable to open rom (\'%s\') for reading: %s", path, strerror(errno));
	
	if (fread(machine->memory + CHIP8_PROGRAM_START, sizeof(*machine->memory), rom_size, f) != rom_size)
		die("unable to read rom (\'%s\'): %s", path, strerror(errno));
}

static inline void load_fonts(struct chip8 *machine)
{
	memcpy(machine->memory + CHIP8_FONT_ADDR, chip8_fonts, sizeof(chip8_fonts));
}

void chip8_init(struct chip8 *machine, const char *rom_path)
{
	srand(time(nullptr));
	*machine = (struct chip8){
		.state = CHIP8_OK,
		.delay_timer = CHIP8_TIMER_FREQ,
		.sound_timer = CHIP8_TIMER_FREQ,
		.pc = CHIP8_PROGRAM_START,
	};

	load_fonts(machine);
	load_rom(machine, rom_path);
}


static void chip8_error(struct chip8 *machine, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	machine->state = CHIP8_ERR;
	vsnprintf(machine->err, CHIP8_ERROR_MAX, fmt, args);

	va_end(args);
}


static inline void clear_display(struct chip8 *machine)
{
	memset(machine->display, 0x0, sizeof(machine->display));
}

static inline uint8_t *get_memory_address(struct chip8 *machine, const uint16_t addr)
{
	if (addr >= CHIP8_MEMORY)
		chip8_error(machine, "invalid memory access at 0x%hX", addr);
	
	return machine->memory + addr;
}

static void draw(struct chip8 *machine,
		const uint8_t x, const uint8_t y, const uint8_t sprite_sz)
{
	const uint8_t *sprite = get_memory_address(machine, machine->i_reg);

	uint8_t pos_x = x;
	uint8_t pos_y = y;
	for (uint8_t i = 0; i < sprite_sz; i++) {
		for (int8_t p = 7; p >= 0; p--) {
			const uint8_t sprite_pixel = (sprite[i] >> p) & 1;

			const uint8_t pixel = machine->display[pos_y][pos_x];

			machine->display[pos_y][pos_x] ^= sprite_pixel;

			if (!machine->display[pos_y][pos_x] && pixel)
				machine->v_reg[REG_VF] = 1;

			pos_x = (pos_x + 1) % CHIP8_DISPLAY_WIDTH;
		}
		pos_x = x;
		pos_y = (pos_y + 1) % CHIP8_DISPLAY_HEIGHT;
	}
}

static inline void get_font(struct chip8 *machine, const uint8_t sprite_id)
{
	machine->i_reg = CHIP8_FONT_ADDR + (CHIP8_FONT_SIZE * sprite_id);
}

static inline uint8_t key_pressed(void)
{
	return getchar();
}

static void subroutine_return(struct chip8 *machine)
{
	if (!machine->sp)
		chip8_error(machine, "stack underflow???");

	machine->pc = machine->stack[--machine->sp];
}

static void subroutine_enter(struct chip8 *machine, const uint16_t addr)
{
	if (machine->sp >= CHIP8_STACK_SIZE)
		chip8_error(machine, "stack overflow");

	machine->stack[machine->sp++] = machine->pc;
	machine->pc = addr;
}

static void store_binary_coded_decimal(struct chip8 *machine, const uint8_t val)
{
	*get_memory_address(machine, machine->i_reg) = (val / 100) % 10;
	*get_memory_address(machine, machine->i_reg + 1) = (val / 10) % 10;
	*get_memory_address(machine, machine->i_reg + 2) = val % 10;
}

/*
	TODO: implement error codes to be stored in chip8 type so the rom
	doesn't have to die
*/

static void register_dump(struct chip8 *machine, const enum chip8_register reg)
{
	uint16_t offset = 0;
	for (enum chip8_register r = REG_V0; r <= reg; r++) {
		uint8_t *mem = get_memory_address(machine, machine->i_reg + offset);
		*mem = machine->v_reg[r];

		offset++;
	}
}

static void register_load(struct chip8 *machine, const enum chip8_register reg)
{
	uint16_t offset = 0;
	for (enum chip8_register r = REG_V0; r <= reg; r++) {
		uint8_t *mem = get_memory_address(machine, machine->i_reg + offset);
		machine->v_reg[r] = *mem;

		offset++;
	}
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

static inline uint16_t read_op_code(const struct chip8 *machine)
{
	return ((uint16_t)machine->memory[machine->pc] << 8) | machine->memory[machine->pc + 1];
}

static void instruct(struct chip8 *machine, const uint16_t op)
{
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

		if (machine->v_reg[reg] == val)
			advance_program_counter(machine);
	} break;

	case 0x4: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		if (machine->v_reg[reg] != val)
			advance_program_counter(machine);
	} break;

	case 0x5: {
		if ((op & 0xF) != 0)
			invalid_opcode(machine, op);

		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		if (machine->v_reg[reg0] == machine->v_reg[reg1])
			advance_program_counter(machine);
	} break;

	case 0x6: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		machine->v_reg[reg] = val;
	} break;

	case 0x7: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		machine->v_reg[reg] += val;
	} break;

	case 0x8: {
		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		switch (op & 0xF) {
		case 0:		machine->v_reg[reg0] = machine->v_reg[reg1]; break;

		case 1:		machine->v_reg[reg0] |= machine->v_reg[reg1]; break;

		case 2:		machine->v_reg[reg0] &= machine->v_reg[reg1]; break;

		case 3:		machine->v_reg[reg0] ^= machine->v_reg[reg1]; break;

		case 4: {
			machine->v_reg[REG_VF] = (0xFF - machine->v_reg[reg1] < machine->v_reg[reg0]);
			machine->v_reg[reg0] += machine->v_reg[reg1];
		} break;

		case 5: {
			machine->v_reg[REG_VF] = (machine->v_reg[reg0] < machine->v_reg[reg1]);
			machine->v_reg[reg0] -= machine->v_reg[reg1];
		} break;

		case 6: {
			machine->v_reg[REG_VF] = machine->v_reg[reg0] & 1;
			machine->v_reg[reg0] >>= machine->v_reg[reg1];
		} break;

		case 7: {
			machine->v_reg[REG_VF] = (machine->v_reg[reg1] < machine->v_reg[reg0]);
			machine->v_reg[reg0] = machine->v_reg[reg1] - machine->v_reg[reg0];
		} break;

		case 0xE: {
			machine->v_reg[REG_VF] = machine->v_reg[reg0] >> 7;
			machine->v_reg[reg0] <<= machine->v_reg[reg1];
		} break;

		default:	invalid_opcode(machine, op);
		}
	} break;

	case 0x9: {
		if ((op & 0xF) != 0)
			invalid_opcode(machine, op);

		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		if (machine->v_reg[reg0] != machine->v_reg[reg1])
			advance_program_counter(machine);
	} break;

	case 0xA: {
		const uint16_t addr = (op & 0x0FFF);

		machine->i_reg = addr;
	} break;

	case 0xB: {
		const uint16_t addr = (op & 0x0FFF);

		machine->pc = machine->v_reg[REG_V0] + addr;
	} break;

	case 0xC: {
		const enum chip8_register reg = ((op >> 8) & 0xF);
		const uint8_t val = (op & 0xFF);

		machine->v_reg[reg] = (rand() & 0xFF) & val;
	} break;

	case 0xD: {
		const enum chip8_register reg0 = ((op >> 8) & 0xF);
		const enum chip8_register reg1 = ((op >> 4) & 0xF);

		const uint8_t sprite_height = (op & 0xF);

		draw(machine, machine->v_reg[reg0], machine->v_reg[reg1], sprite_height);
	} break;

	case 0xE: {
		const uint8_t code = (op & 0xFF);
		const enum chip8_register reg = ((op >> 8) & 0xF);

		switch (code) {
		case 0x9E: {
			if (key_pressed() == machine->v_reg[reg])
				advance_program_counter(machine);
		} break;

		case 0xA1: {
			if (key_pressed() != machine->v_reg[reg])
				advance_program_counter(machine);
		} break;

		default:	invalid_opcode(machine, op);
		}
	} break;

	case 0xF: {
		const uint8_t code = (op & 0xFF);
		const enum chip8_register reg = ((op >> 8) & 0xF);

		switch (code) {
		case 0x07:	machine->v_reg[reg] = machine->delay_timer; break;

		case 0x0A:	machine->v_reg[reg] = key_pressed(); break;

		case 0x15:	machine->delay_timer = machine->v_reg[reg]; break;

		case 0x18:	machine->sound_timer = machine->v_reg[reg]; break;
		
		case 0x1E:	machine->i_reg += machine->v_reg[reg]; break;

		case 0x29:	get_font(machine, machine->v_reg[reg]); break;

		case 0x33:	store_binary_coded_decimal(machine, machine->v_reg[reg]); break;

		case 0x55:	register_dump(machine, reg); break;

		case 0x65:	register_load(machine, reg); break;

		default:	invalid_opcode(machine, op);
		}
	} break;
	}
}

static void tick(struct chip8 *machine)
{
	if (machine->delay_timer)
		machine->delay_timer--;
	if (machine->sound_timer)
		machine->sound_timer--;
	
	usleep((1000 * 1000) / CHIP8_TIMER_FREQ);
}

void chip8_run(struct chip8 *machine)
{
	const uint16_t op = read_op_code(machine);
	if (!op) {
		machine->state = CHIP8_COMPLETE;
		return;
	}

	advance_program_counter(machine);

	instruct(machine, op);

	tick(machine);
}
