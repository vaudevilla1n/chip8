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

static struct token peek_token(struct lexer *lexer)
{
	while(lexer_peek(lexer).type == TOKEN_COMMENT)
		lexer_next(lexer);
	
	return lexer_peek(lexer);
}

static struct token next_token(struct lexer *lexer)
{
	while(lexer_peek(lexer).type == TOKEN_COMMENT)
		lexer_next(lexer);
	
	return lexer_next(lexer);
}

static inline bool check_token(struct lexer *lexer, const enum token_type type)
{
	return peek_token(lexer).type == type;
}

static bool check_operand_token(struct lexer *lexer)
{
	switch (peek_token(lexer).type) {
	case TOKEN_REGISTER:
	case TOKEN_NUMBER:
	case TOKEN_FONT:
	case TOKEN_BCD:
	case TOKEN_MEMORY:
		return true;

	default:
		return false;
	}
}

static enum parse_status parse_operands(struct stmt *stmt, struct lexer *lexer)
{
	stmt->noperands = 0;

	if (!check_operand_token(lexer))
		return PARSE_SUCCESS;

	stmt->operands[stmt->noperands++] = next_token(lexer);

	while (peek_token(lexer).type == TOKEN_COMMA) {
		next_token(lexer);

		if (!check_operand_token(lexer)) {
			const struct token t = next_token(lexer);

			return parser_error(&t, (t.type == TOKEN_INVALID) ? t.as.err : "expected operand");
		}

		if (stmt->noperands >= PARSE_OPERANDS_MAX)
			return parser_error(&stmt->instruction, "too many operands");

		stmt->operands[stmt->noperands++] = next_token(lexer);
	}

	return PARSE_SUCCESS;
}

static enum parse_status parse_instruction(struct stmt *stmt, struct lexer *lexer)
{
	const struct token t = next_token(lexer);

	if (t.type != TOKEN_INSTRUCTION)
		return parser_error(&t, "expected instruction");
	
	stmt->instruction = t;

	return PARSE_SUCCESS;
}

enum parse_status parse_statement(struct stmt *stmt, struct lexer *lexer)
{
	if (peek_token(lexer).type == TOKEN_EOF)
		return PARSE_EOF;

	if (parse_instruction(stmt, lexer) != PARSE_SUCCESS
			|| parse_operands(stmt, lexer) != PARSE_SUCCESS)
		return PARSE_ERROR;

	return PARSE_SUCCESS;
}
