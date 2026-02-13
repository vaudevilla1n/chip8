#include "assembler.h"

#include "file.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "translate.h"
#include "instructions.h"

#include <stdio.h>

static void print_token(const struct token *t)
{
	if (t->type == TOKEN_INVALID)
		printf("%s line %zu, col %zu \'%.*s\' (%s)\n", token_type_to_string(t->type), t->line, t->col, (int)t->textlen, t->text, t->as.err);
	else
		printf("%s line %zu, col %zu \'%.*s\'\n", token_type_to_string(t->type), t->line, t->col, (int)t->textlen, t->text);
}

static void test_lexer(struct file *src)
{
	struct lexer lexer = lexer_new(src->dat, src->len);

	for (;;) {
		const struct token t = lexer_next(&lexer);

		if (t.type == TOKEN_EOF)
			break;

		print_token(&t);
	}
}

static void print_operand(const struct token *operand)
{
	switch (operand->type) {
	case TOKEN_REGISTER:	printf("%s", registers_to_string(operand->as.reg)); break;

	case TOKEN_NUMBER:	printf("0x%hx", operand->as.num); break;

	case TOKEN_FONT:	
	case TOKEN_BCD:		
	case TOKEN_MEMORY:	printf("%.*s", (int)operand->textlen, operand->text); break;

	default:		__builtin_unreachable();
	}
}

static void print_statement(const struct stmt *stmt)
{
	printf("%-8s", instruction_to_string(stmt->instruction.as.ins));
	for (size_t i = 0; i < stmt->noperands; i++) {
		if (i == 1)
			printf(", ");

		const struct token *operand = stmt->operands + i;
		print_operand(operand);
	}
	printf("\n");
}

static void print_opcode(const uint16_t opcode)
{
	printf("(0x%.4hX)\n", opcode);
}

static int write_opcode(struct file *rom, const uint16_t opcode)
{
	if (rom->len + 2 > ROM_SIZE_MAX)
		return 1;

	rom->dat[rom->len] = (opcode >> 8) & 0xFF;
	rom->dat[rom->len + 1] = opcode & 0xFF;
	rom->len += 2;

	return 0;
}

static void report_assembled_rom(const struct file *rom, const char *rom_path)
{
	printf("%zu byte rom assembled and written to \'%s\'\n", rom->len, rom_path);
}

static int test_translation(struct file *src, const char *rom_path)
{
	int err = 0;

	struct lexer lexer = lexer_new(src->dat, src->len);

	struct file rom = {
		.dat = (char[ROM_SIZE_MAX]){ 0 },
		.len = 0,
	};

	struct stmt stmt;
	for (;;) {
		const enum parse_status status = parse_statement(&lexer, &stmt);

		if (status == PARSE_EOF)
			break;

		if (status == PARSE_ERR) {
			err = 1;
			continue;
		}

		print_statement(&stmt);

		uint16_t opcode;
		if (translate_statement_to_opcode(&stmt, &opcode) != TRANSLATE_OK) {
			err = 1;
			continue;
		}

		print_opcode(opcode);

		if (write_opcode(&rom, opcode)) {
			err = 1;
			fprintf(stderr, "rom is too large, truncated to %d bytes\n", ROM_SIZE_MAX);
			break;
		}
	}

	if (write_file(&rom, rom_path)) {
		err = 1;
		perror(rom_path);
	} else {
		report_assembled_rom(&rom, rom_path);
	}

	return err;
}

static int test_assembler(const char *src_path, const char *rom_path)
{
	int err = 0;

	struct file src = read_file(src_path);
	if (!src.dat) {
		perror(src_path);
		return 1;
	}

	test_lexer(&src);

	if (test_translation(&src, rom_path))
		err = 1;

	close_file(&src);

	return err;
}

int assemble_source_file(const char *src_path, const char *rom_path)
{
#ifdef DEBUG
	return test_assembler(src_path, rom_path);
#else
	struct file src = read_file(src_path);
	if (!src.dat) {
		perror(src_path);
		return 1;
	}

	struct lexer lexer = lexer_new(src.dat, src.len);

	struct file rom = {
		.dat = (char[ROM_SIZE_MAX]){ 0 },
		.len = 0,
	};

	int err = 0;

	struct stmt stmt;
	for (;;) {
		const enum parse_status status = parse_statement(&lexer, &stmt);

		if (status == PARSE_EOF)
			break;

		if (status == PARSE_ERR) {
			err = 1;
			continue;
		}

		uint16_t opcode;
		if (translate_statement_to_opcode(&stmt, &opcode) != TRANSLATE_OK) {
			err = 1;
			continue;
		}

		if (write_opcode(&rom, opcode)) {
			fprintf(stderr, "rom is too large, truncated to %d bytes\n", ROM_SIZE_MAX);
			break;
		}
	}

	if (err)
		return err;

	if (write_file(&rom, rom_path)) {
		perror(rom_path);
		err = 1;
	} else {
		report_assembled_rom(&rom, rom_path);
	}

	return err;
#endif
}
