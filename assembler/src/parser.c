#include "parser.h"

#include "common.h"

#include <stdio.h>

/*
	grammar 

	stmt		::= instruction operands?

	instruction	::= not typing allat

	operands	::= operand ( ',' operand )*

	operand		::= font | bcd | address | register | number

	font		::= 'F'
	bcd		::= 'B'

	address		::= '[' I ']'

	register	::= V0-VF, I, ST, DT

	number		::= hexadecimal
	hexadecimal	::= '0x'( [0-9]( [a-f] | [A-F] ) )+
	decimal		::= [0-9]+
	octal		::= '0'[0-7]+
*/

/*
	ast

	stmt	::= instruction operand[]
	operand	::= register | number | special | address
*/

static enum parse_status parser_error(const struct token *t, const char *msg)
{
	fprintf(stderr, "line %zu: col %zu: \'%.*s\': %s\n", t->line, t->col, (int)t->textlen, t->text, msg);
	return PARSE_ERROR;
}

static inline bool check_token(struct lexer *lexer, const enum token_type type)
{
	return lexer_peek(lexer).type == type;
}

static enum parse_status try_parse_operand(struct operand *operand, struct lexer *lexer)
{
	switch (lexer_peek(lexer).type) {
	case TOKEN_REGISTER:		operand->type = OPERAND_REGISTER; break;
	case TOKEN_NUMBER:		operand->type = OPERAND_NUMBER; break;
	case TOKEN_FONT:		operand->type = OPERAND_FONT; break;
	case TOKEN_BCD:			operand->type = OPERAND_BCD; break;
	case TOKEN_MEMORY:		operand->type = OPERAND_MEMORY; break;

	default:			return PARSE_ERROR;
	}

	operand->val = lexer_next(lexer);

	return PARSE_SUCCESS;
}

static enum parse_status parse_operands(struct stmt *stmt, struct lexer *lexer)
{
	stmt->noperands = 0;

	struct operand op;
	if (try_parse_operand(&op, lexer) != PARSE_SUCCESS)
		return PARSE_SUCCESS;

	stmt->operands[stmt->noperands++] = op;

	while (lexer_peek(lexer).type == TOKEN_COMMA) {
		const struct token comma = lexer_next(lexer);

		struct operand op;
		if (try_parse_operand(&op, lexer) != PARSE_SUCCESS)
			return parser_error(&comma, "expected operand");

		if (stmt->noperands >= PARSE_OPERANDS_MAX)
			return parser_error(&stmt->instruction, "too many operands");

		stmt->operands[stmt->noperands++] = op;
	}

	return PARSE_SUCCESS;
}

static enum parse_status parse_instruction(struct stmt *stmt, struct lexer *lexer)
{
	const struct token t = lexer_peek(lexer);

	if (t.type != TOKEN_INSTRUCTION)
		return parser_error(&t, "expected instruction");
	
	stmt->instruction = lexer_next(lexer);

	return PARSE_SUCCESS;
}

enum parse_status parse_statement(struct stmt *stmt, struct lexer *lexer)
{
	if (lexer_peek(lexer).type == TOKEN_EOF)
		return PARSE_EOF;

	if (parse_instruction(stmt, lexer) != PARSE_SUCCESS
			|| parse_operands(stmt, lexer) != PARSE_SUCCESS)
		return PARSE_ERROR;

	return PARSE_SUCCESS;
}
