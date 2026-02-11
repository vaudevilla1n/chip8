#include "parser.h"

#include <stdio.h>

static void parser_error(const struct token *t, const char *msg)
{
	fprintf(stderr, "line %zu: col %zu: \'%.*s\': %s\n", t->line, t->col, (int)t->textlen, t->text, msg);
}

static bool is_operand_token(const enum token_type type)
{
	return (type == TOKEN_REGISTER || type == TOKEN_NUMBER);
}

int parse_stmt(struct stmt *stmt, struct lexer *lexer)
{
	*stmt = (struct stmt){ 0 };

	stmt->instruction = lexer_next(lexer);

	if (stmt->instruction.type != TOKEN_INSTRUCTION) {
		parser_error(&stmt->instruction, "expected instruction");
		return 1;
	}

	if (is_operand_token(lexer_peek(lexer).type))
		stmt->operands[stmt->noperands++] = lexer_next(lexer);

	if (lexer_peek(lexer).type == TOKEN_COMMA) {
		lexer_next(lexer);

		struct token *operand = stmt->operands + stmt->noperands++;
		*operand = lexer_next(lexer);

		if (!is_operand_token(operand->type)) {
			parser_error(operand, "invalid operand");
			return 1;
		}

	}

	return 0;
}
