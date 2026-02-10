#define _DEFAULT_SOURCE
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define unused(x)	(void)(x)
#define todo(x)		do { fprintf(stderr, "todo: \'%s\'\n", (x)); abort(); } while (0)

#define warn(fmt, ...)	do { fprintf(stderr, fmt __VA_OPT__(,)__VA_ARGS__); fputc('\n', stderr); } while (0)
#define die(fmt, ...)	do { warn(fmt __VA_OPT__(,)__VA_ARGS__); exit(1); } while (0)

#define countof(arr)	(sizeof(arr) / sizeof((arr)[0]))


#define CHIP8_MEMORY		0x1000
#define CHIP8_RESERVED_MEMORY	0x200
#define CHIP8_AVAILABLE_MEMORY	0xe00

#define CHIP8_PROGRAM_START	0x200

#define CHIP8_STACK_SIZE	0x10
#define CHIP8_STACK_ADDR	0xEFF

#define CHIP8_TIMER_FREQ	60

#define CHIP8_DISPLAY_HEIGHT	32
#define CHIP8_DISPLAY_WIDTH	64
#define CHIP8_DISPLAY_MEMORY	(CHIP8_DISPLAY_HEIGHT * (CHIP8_DISPLAY_WIDTH / 8))

enum chip8_register {
	REG_V0, REG_V1, REG_V2, REG_V3, REG_V4, REG_V5, REG_V6,
	REG_V7, REG_V8, REG_V9, REG_VA, REG_VB, REG_VC, REG_VD,
	REG_VE, REG_VF, CHIP8_REGISTERS,
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

	uint8_t display[CHIP8_DISPLAY_MEMORY];
} emulator;

void chip8_init(struct chip8 *machine);

int chip8_load_rom(struct chip8 *machine, const char *path);

void chip8_dump_registers(const struct chip8 *machine);
void chip8_diagnostics(const struct chip8 *machine);

void chip8_run(struct chip8 *machine);


[[noreturn]] void usage(void)
{
	die("usage: chip8 ROM");
}

int main(int argc, char **argv)
{
	if (argc != 2)
		usage();

	int err = 0;

	chip8_init(&emulator);

	chip8_diagnostics(&emulator);

	for (int arg = 1; arg < argc; arg++) {
		const char *path = argv[arg];

		printf("\nloading rom (\'%s\')...\n", path);

		if (chip8_load_rom(&emulator, path)) {
			err++;
			continue;
		}

		printf("running program...\n\n");

		chip8_run(&emulator);

		chip8_dump_registers(&emulator);

		printf("\nprogram completed\n");
	}

	return err;
}

void chip8_init(struct chip8 *machine)
{
	srand(time(nullptr));
	*machine = (struct chip8){ 0 };
}

int chip8_load_rom(struct chip8 *machine, const char *path)
{
	struct stat stats;
	if (stat(path, &stats)) {
		warn("unable to get rom's (\'%s\') size: %s", path, strerror(errno));
		return 1;
	}
	
	if (stats.st_size > CHIP8_AVAILABLE_MEMORY) {
		warn("rom (\'%s\') is too large", path);
		return 1;
	}

	const uint16_t rom_size = stats.st_size; 
	
	FILE *f = fopen(path, "r");
	if (!f) {
		warn("unable to open rom (\'%s\') for reading: %s", path, strerror(errno));
		return 1;
	}
	
	if (fread(machine->memory + CHIP8_PROGRAM_START, sizeof(*machine->memory), rom_size, f) != rom_size) {
		warn("unable to read rom (\'%s\'): %s", path, strerror(errno));
		return 1;
	}

	return 0;
}

void chip8_dump_registers(const struct chip8 *machine)
{
	printf("registers\n"); 

	printf("PC: 0x%.3hx\n", machine->pc);
	printf("SP: 0x%.3hx\n", machine->sp);
	printf("I: 0x%.3hx\n", machine->i_reg);

	for (enum chip8_register reg = REG_V0; reg < CHIP8_REGISTERS; reg++)
		printf("V%X: 0x%.2hhx\n", reg, machine->v_reg[reg]); 
}

void chip8_diagnostics(const struct chip8 *machine)
{
	printf("(chip8)\n");
	printf("total data registers: %zu\n", sizeof(machine->v_reg));
	printf("total available memory: %zu\n", sizeof(machine->memory));
	printf("total stack memory: %zu\n", sizeof(machine->stack));
	printf("display (%zu x %zu)\n", sizeof(machine->display[0]), countof(machine->display));
}

static inline void clear_display(struct chip8 *machine)
{
	memset(machine->display, 0x0, sizeof(machine->display));
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
	if (!machine->sp)
		die("stack underflow???");

	machine->pc = machine->stack[--machine->sp];
}

static void subroutine_enter(struct chip8 *machine, const uint16_t addr)
{
	if (machine->sp >= CHIP8_STACK_SIZE)
		die("stack overflow");

	machine->stack[machine->sp++] = machine->pc;
	machine->pc = addr;
}

static void store_binary_coded_decimal(struct chip8 *machine, const uint8_t val)
{
	unused(val);
	unused(machine);
	todo("store_binary_coded_decimal");
}

/*
	TODO: implement error codes to be stored in chip8 type so the rom
	doesn't have to die
*/

static inline uint8_t *get_memory_address(struct chip8 *machine, const uint16_t addr)
{
	if (addr >= CHIP8_MEMORY)
		die("invalid memory access at 0x%hX", addr);
	
	return machine->memory + addr;
}

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

		draw(machine->v_reg[reg0], machine->v_reg[reg1], sprite_height);
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

		case 0x29:	machine->i_reg = sprite_address(machine->v_reg[reg]); break;

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
	machine->delay_timer = CHIP8_TIMER_FREQ;
	machine->sound_timer = CHIP8_TIMER_FREQ;

	machine->pc = CHIP8_PROGRAM_START;
	machine->sp = 0;

	machine->i_reg = 0x000;
	memset(machine->v_reg, 0x0, sizeof(machine->v_reg));

	for (;;) {
		const uint16_t op = read_op_code(machine);
		if (!op)
			break;

		advance_program_counter(machine);

		instruct(machine, op);

		tick(machine);
	}
}
