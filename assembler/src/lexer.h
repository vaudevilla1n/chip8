#pragma once

#include "token.h"

#include <stddef.h>

struct lexer {
	const char *src;
	size_t srclen;

	size_t pos;

	size_t col;
	size_t line;

	struct token token;
};

struct lexer lexer_new(const char *src, const size_t srclen);
struct token lexer_next(struct lexer *lexer);
struct token lexer_peek(struct lexer *lexer);
