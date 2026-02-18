#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

#define OPCODE_SIZE	2

/*
	A -> memory address operand
	V -> byte register operand
	B -> byte value operand
	I -> I register operand
	M -> memory ([I]) operand
	S -> ST register operand
	D -> DT register operand
	K -> pressed key operand
	F -> font operand
	BCD -> bcd operand
*/
enum chip8_opcode: uint16_t {
	OP_SYS		= 0x0000,
	OP_CLS		= 0x00E0,
	OP_RET		= 0x00EE,

	OP_JP_A		= 0x1000,
	OP_JP_VA	= 0xB000,

	OP_CALL		= 0x2000,

	OP_SE_VB	= 0x3000,
	OP_SE_VV	= 0x5000,

	OP_SNE_VB	= 0x4000,
	OP_SNE_VV	= 0x9000,

	OP_OR		= 0x8001,
	OP_AND		= 0x8002,
	OP_XOR		= 0x8003,
	OP_SUB		= 0x8005,
	OP_SHR		= 0x8006,
	OP_SUBN		= 0x8007,
	OP_SHL		= 0x800E,

	OP_RND		= 0xC000,

	OP_DRW		= 0xD000,

	OP_SKP		= 0xE09E,
	OP_SKNP		= 0xE0A1,

	OP_ADD_VB	= 0x7000,
	OP_ADD_VV	= 0x8004,
	OP_ADD_IV	= 0xF01E,

	OP_LD_VB	= 0x6000,
	OP_LD_VV	= 0x8000,
	OP_LD_IA	= 0xA000,
	OP_LD_VD	= 0xF007,
	OP_LD_VK	= 0xF00A,
	OP_LD_DV	= 0xF015,
	OP_LD_SV	= 0xF018,
	OP_LD_FV	= 0xF029,
	OP_LD_BCDV	= 0xF033,
	OP_LD_MV	= 0xF055,
	OP_LD_VM	= 0xF065,

	OP_ERR		= 0xFFFF,
};

int disassemble(FILE *rom, FILE *src);

#define DEFAULT_OUTPUT_PATH	"./"

FILE *xfopen(const char *path, const char *mode)
{
	FILE *f = fopen(path, mode);

	if (!f) {
		perror(path);
		exit(1);
	}

	return f;
}

[[noreturn]] void usage(void)
{
	fprintf(stderr, "usage: ./cdasm [-o FILE] ROM\n");
	exit(1);
}

int main(int argc, char **argv)
{
	const int opt = getopt(argc, argv, "o:");

	const char *output_path = nullptr;

	switch (opt) {
	case 'o':	output_path = optarg; break;
	case -1:	break;

	default:	usage();
	}

	if (optind + 1 != argc)
		usage();
	
	int err = 0;
	
	const char *input_path = argv[optind]; 

	FILE *rom = xfopen(input_path, "r");
	FILE *src = output_path ? xfopen(output_path, "w") : stdout;

	err = disassemble(rom, src);

	if (err)
		fprintf(stderr, "unsuccessful disassembly\n");

	fclose(rom);
	fclose(src);
}

static inline int read_opcode(FILE *rom, uint16_t *op)
{
	const long pos = ftell(rom);

	uint8_t buf[2];
	const size_t r = fread(buf, sizeof(*buf), sizeof(buf), rom);

	if (r == OPCODE_SIZE) {
		*op = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
		return 0;
	}
	
	if (!r)
		return 1;

	fprintf(stderr, "pos %ld: broken opcode: read %zu %s out of %d: \'0x%x\'\n",
			pos, r, r == 1 ? "byte" : "bytes", OPCODE_SIZE, buf[0]);
	return 1;
}

#define STATEMENT_MAX	64

static inline void write_statement(char *stmt, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	vsnprintf(stmt, STATEMENT_MAX, fmt, args);

	va_end(args);
}

static inline uint8_t reg0(const uint16_t op)
{
	return (op >> 8) & 0xF;
}

static inline uint8_t reg1(const uint16_t op)
{
	return (op >> 4) & 0xF;
}

static inline uint16_t addr(const uint16_t op)
{
	return (op & 0x0FFF);
}

static inline uint8_t byte(const uint16_t op)
{
	return (op & 0x00FF);
}

static inline void write_address_statement(char *stmt, const char *ins, const uint16_t op)
{
	write_statement(stmt, "%s 0x%.3hX", ins, addr(op));
}

static inline void write_register_statement(char *stmt, const char *ins, const uint16_t op)
{
	write_statement(stmt, "%s V%X", ins, reg0(op));
}

static inline void write_byte_statement(char *stmt, const char *ins, const uint16_t op)
{
	write_statement(stmt, "%s V%X, 0x%.2hhX", ins, reg0(op), byte(op));
}

static inline void write_arithmetic_statement(char *stmt, const char *ins, const uint16_t op)
{
	write_statement(stmt, "%s V%X, V%X", ins, reg0(op), reg1(op));
}

static inline void write_left_special_statement(char *stmt, const char *ins, const char *special, const uint16_t op)
{
	write_statement(stmt, "%s %s, V%X", ins, special, reg0(op));
}

static inline void write_right_special_statement(char *stmt, const char *ins, const char *special, const uint16_t op)
{
	write_statement(stmt, "%s V%X, %s", ins, reg0(op), special);
}

static const char *craft_statement(const uint16_t op)
{
	static char stmt[STATEMENT_MAX];

	switch (op & 0xF000) {
	case 0x0000: {
		switch (op & 0x0FFF) {
		case OP_CLS:	write_statement(stmt, "cls"); break;
		case OP_RET:	write_statement(stmt, "ret"); break;
		case OP_SYS:	write_address_statement(stmt, "sys", op); break;

		default:	return nullptr;
		}
	} break;

	case 0x8000: {
		switch (op & 0xF00F) {
		case OP_LD_VV:	write_arithmetic_statement(stmt, "ld", op); break;
		case OP_OR:	write_arithmetic_statement(stmt, "or", op); break;
		case OP_AND:	write_arithmetic_statement(stmt, "and", op); break;
		case OP_XOR:	write_arithmetic_statement(stmt, "xor", op); break;
		case OP_ADD_VV:	write_arithmetic_statement(stmt, "add", op); break;
		case OP_SUB:	write_arithmetic_statement(stmt, "sub", op); break;
		case OP_SHR:	write_arithmetic_statement(stmt, "shr", op); break;
		case OP_SUBN:	write_arithmetic_statement(stmt, "subn", op); break;
		case OP_SHL:	write_arithmetic_statement(stmt, "shl", op); break;

		default:	return nullptr;
		}
	} break;

	case 0xE000: {
		switch (op & 0xF0FF) {
		case OP_SKP:	write_register_statement(stmt, "skp", op); break;
		case OP_SKNP:	write_register_statement(stmt, "sknp", op); break;

		default:	return nullptr;
		}
	} break;

	case 0xF000: {
		switch (op & 0xF0FF) {
		case OP_LD_VD:		write_right_special_statement(stmt, "ld", "D", op); break;
		case OP_LD_VK:		write_right_special_statement(stmt, "ld", "K", op); break;

		case OP_LD_DV:		write_left_special_statement(stmt, "ld", "D", op); break;
		case OP_LD_SV:		write_left_special_statement(stmt, "ld", "S", op); break;
		case OP_LD_FV:		write_left_special_statement(stmt, "ld", "F", op); break;
		case OP_LD_BCDV:	write_left_special_statement(stmt, "ld", "B", op); break;

		case OP_LD_MV:		write_left_special_statement(stmt, "ld", "[I]", op); break;
		case OP_LD_VM:		write_right_special_statement(stmt, "ld", "[I]", op); break;

		case OP_ADD_IV:		write_left_special_statement(stmt, "add", "I", op); break;

		default:		return nullptr;
		}
	} break;

	case OP_JP_A:	write_address_statement(stmt, "jp", op); break;
	case OP_JP_VA:	write_statement(stmt, "jp V0, 0x%hX", addr(op)); break;

	case OP_CALL:	write_address_statement(stmt, "call", op); break;

	case OP_SE_VB:	write_byte_statement(stmt, "se", op); break;
	case OP_SE_VV:	write_register_statement(stmt, "se", op); break;

	case OP_SNE_VB:	write_byte_statement(stmt, "sne", op); break;
	case OP_SNE_VV:	write_register_statement(stmt, "sne", op); break;

	case OP_RND:	write_byte_statement(stmt, "rnd", op); break;
	case OP_DRW:	write_statement(stmt, "drw V%hX, V%hX, 0x%hhX", reg0(op), reg1(op), (op & 0x000F)); break;

	case OP_ADD_VB:	write_byte_statement(stmt, "add", op); break;

	case OP_LD_VB:	write_byte_statement(stmt, "ld", op); break;
	case OP_LD_IA:	write_statement(stmt, "ld I, 0x%.3hX", addr(op)); break;

	default:	return nullptr;
	}

	return stmt;
}

int disassemble(FILE *rom, FILE *src)
{
	uint16_t op;

	for (;;) {
		const long pos = ftell(rom);

		if (read_opcode(rom, &op))
			break;

#ifdef DEBUG
		fprintf(stderr, "0x%hx\n", op);
#endif

		const char *stmt = craft_statement(op);

		if (!stmt) {
			fprintf(stderr, "pos %ld: invalid opcode: 0x%hX\n", pos, op);
			continue;
		}

		fprintf(src, "\t%s\n", stmt);
	}

	return 0;
}
