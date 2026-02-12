#include "assembler.h"

#include "file.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
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

	default:	__builtin_unreachable();
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

static void test_parser(struct file *src)
{
	struct lexer lexer = lexer_new(src->dat, src->len);

	struct stmt stmt;
	for (;;) {
		const enum parse_status status = parse_statement(&stmt, &lexer);

		if (status == PARSE_EOF)
			break;

		if (status == PARSE_ERROR)
			continue;

		print_statement(&stmt);
	}
}

int assemble_source_file(const char *path)
{
	struct file src = read_file(path);

	if (!src.dat) {
		perror(path);
		return 1;
	}

	instruction_lookup_table_init();

	test_lexer(&src);
	test_parser(&src);

	close_file(&src);

	return 0;
}
