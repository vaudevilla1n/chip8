#include "assembler.h"

#include "file.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "instructions.h"

#include <stdio.h>

int assemble_source_file(const char *path)
{
	struct file src = read_file(path);

	if (!src.dat) {
		perror(path);
		return 1;
	}

	instruction_lookup_table_init();

	/*
		TODO: wrap this in assembler header main
	*/

	struct lexer lexer = lexer_new(src.dat, src.len);

	for (;;) {
		const struct token t = lexer_next(&lexer);

		if (t.type == TOKEN_EOF)
			break;

		if (t.type == TOKEN_INVALID)
			printf("%s line %zu, col %zu \'%.*s\' (%s)\n", token_type_to_string(t.type), t.line, t.col, (int)t.textlen, t.text, t.as.err);
		else
			printf("%s line %zu, col %zu \'%.*s\'\n", token_type_to_string(t.type), t.line, t.col, (int)t.textlen, t.text);
	}

	/*

	struct stmt stmt;
	for (;;) {
		const enum parse_status status = parse_stmt(&stmt, &lexer);

		if (status == PARSE_EOF)
			break;

		if (status == PARSE_ERROR)
			continue;

		const uint16_t op = opcode_from_statement(&stmt);
		printf("(0x%.4hx) ", op);

		printf("%-8s", instruction_to_string(stmt.instruction.as.ins));
		for (size_t i = 0; i < stmt.noperands; i++) {
			if (i == 1)
				printf(", ");

			struct token *operand = stmt.operands + i;

			switch (operand->type) {
			case TOKEN_REGISTER:	printf("%s", registers_to_string(operand->as.reg)); break;
			case TOKEN_NUMBER:	printf("0x%hx", operand->as.num); break;

			default:	__builtin_unreachable();
			}
		}
		printf("\n");
	}
	*/

	close_file(&src);

	return 0;
}
