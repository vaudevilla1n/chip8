#include "lexer.h"

#include "common.h"
#include "registers.h"
#include "instructions.h"

#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

struct lexer lexer_new(const char *src, const size_t srclen)
{
	return (struct lexer){
		.src = src,
		.srclen = srclen,
	};
}

static char peek_char(const struct lexer *lexer)
{
	return (lexer->pos < lexer->srclen) ? lexer->src[lexer->pos] : '\0';
}

static char next_char(struct lexer *lexer)
{
	if (lexer->pos >= lexer->srclen)
		return '\0';

	const char c = lexer->src[lexer->pos++];

	if (c == '\n') {
		lexer->col = 0;
		lexer->line++;
	} else {
		lexer->col++;
	}

	return c;
}

static void invalid_token(struct token *t, const char *err)
{
	t->type = TOKEN_INVALID;
	t->as.err = err;
}

static inline bool ishexdigit(const char d)
{
	return (isdigit(d) || ('a' <= d && d <= 'f') || ('A' <= d && d <= 'F'));
}

static inline bool isoctdigit(const char d)
{
	return ('0' <= d && d <= '7');
}

static void lex_number(struct lexer *lexer, struct token *t)
{
	t->type = TOKEN_NUMBER;

	const size_t start = lexer->pos;

	const char delim = next_char(lexer);
	if (delim == '0' && (peek_char(lexer) == 'x' || peek_char(lexer) == 'X')) {
		next_char(lexer);

		while (ishexdigit(peek_char(lexer)))
			next_char(lexer);
	} else if (delim == '0') {
		next_char(lexer);

		while (isoctdigit(peek_char(lexer)))
			next_char(lexer);
	} else {
		while (isdigit(peek_char(lexer)))
			next_char(lexer);
	}

	t->textlen = lexer->pos - start;

	const uint32_t x = strtoul(t->text, nullptr, 0);

	if (errno == ERANGE || x > USHRT_MAX)
		invalid_token(t, "number literal out of range");
	else
		t->as.num = (uint16_t)x;
}

static bool set_register_token(struct token *t)
{
	const enum registers reg = register_lookup(t->text, t->textlen);

	if (reg == REG_INVALID)
		return false;
	
	t->type = TOKEN_REGISTER;
	t->as.reg = reg;

	return true;
}

static bool set_instruction_token(struct token *t)
{
	const enum instruction ins = instruction_lookup(t->text, t->textlen);

	if (ins == INS_INVALID)
		return false;
	
	t->type = TOKEN_INSTRUCTION;
	t->as.ins = ins;

	return true;
}

static void lex_keyword(struct lexer *lexer, struct token *t)
{
	const size_t start = lexer->pos;

	while (isalnum(peek_char(lexer)))
		next_char(lexer);

	t->textlen = lexer->pos - start;
	
	if (set_register_token(t))
		return;

	if (set_instruction_token(t))
		return;

	invalid_token(t, "unknown identifier");
}

struct token lexer_next(struct lexer *lexer)
{
	while (isspace(peek_char(lexer)))
		next_char(lexer);
	
	struct token t = {
		.text = lexer->src + lexer->pos,

		.col = lexer->col,
		.line = lexer->line,
	};

	const char c = peek_char(lexer);

	switch (c) {
	case '\0': {
		t.type = TOKEN_EOF;
		t.textlen = 0;
	} break;

	case ',': {
		t.type = TOKEN_COMMA;
		t.textlen = 1;

		next_char(lexer);
	} break;

	default: {
		if (isdigit(c)) {
			lex_number(lexer, &t);
		} else if (isalpha(c)) {
			lex_keyword(lexer, &t);
		} else {
			t.textlen = 1;
			invalid_token(&t, "unknown token");

			next_char(lexer);
		}
	} break;
	}

	return t;
}

struct token lexer_peek(struct lexer *lexer)
{
	const size_t pos = lexer->pos;
	const size_t col = lexer->col;
	const size_t line = lexer->line;

	struct token t = lexer_next(lexer);

	lexer->pos = pos;
	lexer->col = col;
	lexer->line = line;

	return t;
}

