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

static const char *craft_statement(const uint16_t op)
{
	static char stmt[STATEMENT_MAX];

	switch (op & 0xF000) {
	case 0x0: {
		switch (op & 0x0FFF) {
		case OP_CLS:	write_statement(stmt, "cls"); break;
		case OP_RET:	write_statement(stmt, "ret"); break;

		default:	write_statement(stmt, "sys"); break;
		}
	} break;

	case OP_JP_A:	write_statement(stmt, "jp"); break;
	case OP_JP_VA:	write_statement(stmt, "jp"); break;

	case OP_CALL:	write_statement(stmt, "call"); break;

	case OP_SE_VB:	write_statement(stmt, "se"); break;
	case OP_SE_VV:	write_statement(stmt, "se"); break;

	case OP_SNE_VB:	write_statement(stmt, "sne"); break;
	case OP_SNE_VV:	write_statement(stmt, "sne"); break;

	case OP_RND:	write_statement(stmt, "rnd"); break;
	case OP_DRW:	write_statement(stmt, "drw"); break;

	case OP_LD_VB:	write_statement(stmt, "ld"); break;
	case OP_LD_IA:	write_statement(stmt, "ld"); break;

	default:	return nullptr;
	}

	return stmt;
}

int disassemble(FILE *rom, FILE *src)
{
	uint16_t op;

	for (;;) {
		if (read_opcode(rom, &op))
			break;

		printf("0x%hx\n", op);

		const char *stmt = craft_statement(op);
		printf("\t%s\n", stmt);
	}

	return 0;
}
