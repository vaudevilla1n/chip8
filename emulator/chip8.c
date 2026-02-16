#define _DEFAULT_SOURCE

#include <time.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curses.h>
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

#define CHIP8_FONT_ADDR		0x000
#define CHIP8_FONT_SIZE		5
#define CHIP8_TOTAL_FONTS	16

#define CHIP8_DISPLAY_HEIGHT	32
#define CHIP8_DISPLAY_WIDTH	64

#define CHIP8_DISPLAY_X		((COLS - CHIP8_DISPLAY_WIDTH) / 2)
#define CHIP8_DISPLAY_Y		((LINES - CHIP8_DISPLAY_HEIGHT) / 2)

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

	uint8_t display[CHIP8_DISPLAY_HEIGHT][CHIP8_DISPLAY_WIDTH];
} emulator;

void chip8_init(struct chip8 *machine, const char *rom_path);

void chip8_dump_registers(const struct chip8 *machine);
void chip8_diagnostics(const struct chip8 *machine);

bool chip8_run(struct chip8 *machine);


void tui_init(void);
void tui_draw_display_border(void);
void tui_draw_display(const struct chip8 *machine);
void tui_draw_register_info(const struct chip8 *machine);
void tui_deinit(void);

#define tui_info(fmt, ...)	mvprintw(LINES - 2, 0, fmt __VA_OPT__(,)__VA_ARGS__)


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
		
		if (chip8_run(&emulator)) {
			tui_draw_display(&emulator);
			tui_draw_register_info(&emulator);
		} else {
			tui_info("program completed");
		}
	}

	tui_deinit();
}

#define	BLACK	1
#define	WHITE	2

void tui_init(void)
{
	initscr();
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

void tui_draw_display_border(void)
{
	draw_box(CHIP8_DISPLAY_Y - 1, CHIP8_DISPLAY_X - 1, CHIP8_DISPLAY_HEIGHT + 2, CHIP8_DISPLAY_WIDTH + 2);
	mvprintw(LINES - 1, 0, "%d x %d display", COLS, LINES);
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
		.delay_timer = CHIP8_TIMER_FREQ,
		.sound_timer = CHIP8_TIMER_FREQ,
		.pc = CHIP8_PROGRAM_START,
	};

	load_fonts(machine);
	load_rom(machine, rom_path);
}

static inline void clear_display(struct chip8 *machine)
{
	memset(machine->display, 0x0, sizeof(machine->display));
}

static inline uint8_t *get_memory_address(struct chip8 *machine, const uint16_t addr)
{
	if (addr >= CHIP8_MEMORY)
		tui_info("invalid memory access at 0x%hX", addr);
	
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

static void get_font(struct chip8 *machine, const uint8_t sprite_id)
{
	machine->i_reg = CHIP8_FONT_ADDR + (CHIP8_FONT_SIZE * sprite_id);
}

static uint8_t key_pressed(void)
{
	nodelay(stdscr, false);
	const uint8_t k = getch();
	nodelay(stdscr, true);

	return k;
}

static void subroutine_return(struct chip8 *machine)
{
	if (!machine->sp)
		tui_info("stack underflow???");

	machine->pc = machine->stack[--machine->sp];
}

static void subroutine_enter(struct chip8 *machine, const uint16_t addr)
{
	if (machine->sp >= CHIP8_STACK_SIZE)
		tui_info("stack overflow");

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

bool chip8_run(struct chip8 *machine)
{
	const uint16_t op = read_op_code(machine);
	if (!op)
		return false;

	advance_program_counter(machine);

	instruct(machine, op);

	tick(machine);
	
	return true;
}
