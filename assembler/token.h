#pragma once

#include <stddef.h>

enum token_type {
	TOKEN_EOF,
	TOKEN_INVALID,
	TOKEN_COMMA,
	TOKEN_NUMBER,
	TOKEN_REGISTER,
	TOKEN_INSTRUCTION,
};

const char *token_type_string(const enum token_type type);

struct token {
	enum token_type type;

	const char *text;
	size_t textlen;

	size_t col;
	size_t line;
};
