#include "parser.h"

#include "token.h"
#include "common.h"

#include <stdio.h>

/*
	grammar 

	stmt		::= instruction operands? eol

	eol		::= '\n'

	instruction	::= not typing allat

	operands	::= operand ( ',' operand )*

	operand		::= key | font | bcd | address | register | number

	key		::= 'K'
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
	token_error(t, msg);
	return PARSE_ERR;
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
	case TOKEN_KEY:
	case TOKEN_MEMORY:
		return true;

	default:
		return false;
	}
}

static enum parse_status parse_operands(struct lexer *lexer, struct stmt *stmt)
{
	stmt->noperands = 0;

	if (!check_operand_token(lexer))
		return PARSE_OK;

	stmt->operands[stmt->noperands++] = next_token(lexer);

	while (peek_token(lexer).type == TOKEN_COMMA) {
		next_token(lexer);

		if (!check_operand_token(lexer)) {
			const struct token t = next_token(lexer);

			return parser_error(&t, (t.type == TOKEN_INVALID) ? t.as.err : "expected operand");
		}

		if (stmt->noperands >= INSTRUCTION_OPERAND_MAX)
			return parser_error(&stmt->instruction, "too many operands");

		stmt->operands[stmt->noperands++] = next_token(lexer);
	}

	return PARSE_OK;
}

static enum parse_status parse_instruction(struct lexer *lexer, struct stmt *stmt)
{
	const struct token t = next_token(lexer);

	if (t.type != TOKEN_INSTRUCTION)
		return parser_error(&t, "expected instruction");
	
	stmt->instruction = t;

	return PARSE_OK;
}

static void skip_empty_lines(struct lexer *lexer)
{
	while (peek_token(lexer).type == TOKEN_EOL)
		next_token(lexer);
}

enum parse_status parse_statement(struct lexer *lexer, struct stmt *stmt)
{
	skip_empty_lines(lexer);

	if (peek_token(lexer).type == TOKEN_EOF)
		return PARSE_EOF;

	if (parse_instruction(lexer, stmt) != PARSE_OK
			|| parse_operands(lexer, stmt) != PARSE_OK)
		return PARSE_ERR;
	
	const struct token t = next_token(lexer);
	if (t.type != TOKEN_EOL)
		return parser_error(&t, "junk at end of line");

	skip_empty_lines(lexer);

	return PARSE_OK;
}
