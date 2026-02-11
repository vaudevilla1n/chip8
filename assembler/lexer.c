#include "lexer.h"

#include "common.h"

#include <ctype.h>

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

static inline bool ishexdigit(const char d)
{
	return (isdigit(d) || ('a' <= d && d <= 'f') || ('A' <= d && d <= 'F'));
}

static inline bool isoctdigit(const char d)
{
	return ('0' <= d && d <= '7');
}

static void lex_number(struct lexer *lexer)
{
	const char delim = next_char(lexer);

	if (delim == '0' && (peek_char(lexer) == 'x' || peek_char(lexer) == 'X')) {
		while (ishexdigit(peek_char(lexer)))
			next_char(lexer);
	} else if (delim == '0') {
		while (isoctdigit(peek_char(lexer)))
			next_char(lexer);
	} else {
		while (isdigit(peek_char(lexer)))
			next_char(lexer);
	}
}

static void lex_keyword(struct lexer *lexer, struct token *t)
{
	unused(t);

	while (isalnum(peek_char(lexer)))
		next_char(lexer);
}

struct token lexer_next(struct lexer *lexer)
{
	struct token t = {
		.type = TOKEN_INVALID,
		
		.text = lexer->src + lexer->pos,
		.textlen = 0,

		.col = lexer->col,
		.line = lexer->line,
	};

	while (isspace(peek_char(lexer)))
		next_char(lexer);
	
	const char c = peek_char(lexer);

	const size_t start = lexer->pos;

	switch (c) {
	case '\0': {
		t.type = TOKEN_EOF;
	} break;

	case ',': {
		t.type = TOKEN_COMMA;
		next_char(lexer);
	} break;

	default: {
		if (isdigit(c)) {
			t.type = TOKEN_NUMBER;
			lex_number(lexer);
		} else if (isalpha(c)) {
			t.type = TOKEN_INSTRUCTION;
			lex_keyword(lexer, &t);
		}
	} break;
	}

	t.textlen = lexer->pos - start;

	return t;
}
