#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define unused(x)	(void)(x)
#define todo(x)		do { fprintf(stderr, "todo: \'%s\'\n", (x)); abort(); } while (0)

#define warn(fmt, ...)	do { fprintf(stderr, fmt __VA_OPT__(,)__VA_ARGS__); fputc('\n', stderr); } while (0)
#define die(fmt, ...)	do { warn(fmt __VA_OPT__(,)__VA_ARGS__); exit(1); } while (0)

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
} emulator;

void chip8_init(struct chip8 *machine);
void chip8_dump_registers(const struct chip8 *machine);
void chip8_diagnostics(const struct chip8 *machine);
void chip8_run(struct chip8 *machine);

struct chip8_executable {
	uint8_t *data;
	uint16_t size;
};

struct chip8_executable read_executable(const char *path);
void load_executable(struct chip8 *machine, const struct chip8_executable *exe);
void close_executable(struct chip8_executable *exe);

[[noreturn]] void usage(void)
{
	fprintf(stderr, "chip8 [EXECUTABLE]...\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int err = 0;

	chip8_init(&emulator);

	chip8_diagnostics(&emulator);

	for (int arg = 1; arg < argc; arg++) {
		const char *path = argv[arg];

		struct chip8_executable program = read_executable(path);
		
		if (!program.data) {
			err++;
			continue;
		}

		load_executable(&emulator, &program);

		printf("\nrunning program \'%s\'\n\n", path);
		chip8_run(&emulator);

		chip8_dump_registers(&emulator);

		printf("\nprogram completed\n");
		close_executable(&program);
	}

	if (err)
		usage();
}


struct chip8_executable read_executable(const char *path)
{
	struct chip8_executable exe = { 0 };

	struct stat stats;
	if (stat(path, &stats)) {
		warn("unable to get executable's (\'%s\') size: %s", path, strerror(errno));
		return (struct chip8_executable){ 0 };
	}
	
	if (stats.st_size > CHIP8_MEMORY) {
		warn("executable (\'%s\') is too large", path);
		return (struct chip8_executable){ 0 };
	}
	
	FILE *f = fopen(path, "r");
	if (!f) {
		warn("unable to open executable (\'%s\') for reading: %s", path, strerror(errno));
		goto cleanup_file;
	}
		
	exe.size = stats.st_size;
	exe.data = calloc(exe.size, sizeof(exe.data[0]));

	if (!exe.data) {
		warn("unable to allocate memory for executable (\'%s\') data: %s", path, strerror(errno));
		return (struct chip8_executable){ 0 };
	}
	
	if (fread(exe.data, sizeof(exe.data[0]), exe.size, f) != exe.size) {
		warn("unable to read executable (\'%s\'): %s", path, strerror(errno));
		goto cleanup_memory;
	}
	
	return exe;

cleanup_memory:
	free(exe.data);
cleanup_file:
	fclose(f);

	return (struct chip8_executable){ 0 };
}

void load_executable(struct chip8 *machine, const struct chip8_executable *exe)
{
	if (exe->size)
		memcpy(machine->memory, exe->data, exe->size);
}

void close_executable(struct chip8_executable *exe)
{
	free(exe->data);
	*exe = (struct chip8_executable){ 0 };
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

/*
	TODO: implement error codes to be stored in chip8 type so the program
	doesn't have to die
*/

static inline uint8_t *get_memory_address(struct chip8 *machine, const uint16_t addr)
{
	if (addr >= CHIP8_STACK_MEMORY)
		die("invalid memory access at 0x%hX", addr);
	
	return machine->stack + addr;
}

static void register_dump(struct chip8 *machine, const enum chip8_register reg)
{
	uint16_t offset = 0;
	for (enum chip8_register r = REG_V0; r <= reg; r++) {
		uint8_t *mem = get_memory_address(machine, machine->i_reg + offset);
		*mem = machine->reg[r];

		offset++;
	}
}

static void register_load(struct chip8 *machine, const enum chip8_register reg)
{
	uint16_t offset = 0;
	for (enum chip8_register r = REG_V0; r <= reg; r++) {
		uint8_t *mem = get_memory_address(machine, machine->i_reg + offset);
		machine->reg[r] = *mem;

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

static void instruct(struct chip8 *machine)
{
	const uint16_t op = read_op_code(machine);
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

		case 4: {
			machine->reg[REG_VF] = (0xFF - machine->reg[reg1] < machine->reg[reg0]);
			machine->reg[reg0] += machine->reg[reg1];
		} break;

		case 5: {
			machine->reg[REG_VF] = (machine->reg[reg0] < machine->reg[reg1]);
			machine->reg[reg0] -= machine->reg[reg1];
		} break;

		case 6: {
			machine->reg[REG_VF] = machine->reg[reg0] & 1;
			machine->reg[reg0] >>= machine->reg[reg1];
		} break;

		case 7: {
			machine->reg[REG_VF] = (machine->reg[reg1] < machine->reg[reg0]);
			machine->reg[reg0] = machine->reg[reg1] - machine->reg[reg0];
		} break;

		case 0xE: {
			machine->reg[REG_VF] = machine->reg[reg0] >> 7;
			machine->reg[reg0] <<= machine->reg[reg1];
		} break;

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

		case 0x55:	register_dump(machine, reg); break;

		case 0x65:	register_load(machine, reg); break;

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

void chip8_run(struct chip8 *machine)
{
	machine->pc = 0x0000;

	memset(machine->reg, 0x0, sizeof(machine->reg));
	memset(machine->stack, 0x0, CHIP8_STACK_MEMORY);

	machine->i_reg = 0x000;

	while (read_op_code(machine))
		instruct(machine);
}

void chip8_dump_registers(const struct chip8 *machine)
{
	printf("registers\n"); 
	for (enum chip8_register reg = REG_V0; reg < CHIP8_DATA_REGISTERS; reg++)
		printf("V%X: 0x%.2hhx\n", reg, machine->reg[reg]); 
	printf("I: 0x%.4hx\n", machine->i_reg);
}

void chip8_diagnostics(const struct chip8 *machine)
{
	printf("(chip8)\n");
	printf("total data registers: %zu\n", sizeof(machine->reg));
	printf("total available memory: %zu\n", sizeof(machine->memory));
	printf("total stack memory: %zu\n", sizeof(machine->stack));
	printf("display (%zu x %zu)\n", sizeof(machine->display[0]), countof(machine->display));

	chip8_dump_registers(machine);
}
