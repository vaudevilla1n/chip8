#pragma once

#include "registers.h"
#include "instructions.h"

#include <stddef.h>
#include <stdint.h>

enum token_type {
	TOKEN_EOF,
	TOKEN_INVALID,

	TOKEN_COMMENT,

	TOKEN_COMMA,
	TOKEN_LBRACKET,
	TOKEN_RBRACKET,

	TOKEN_BCD,
	TOKEN_FONT,

	TOKEN_MEMORY,

	TOKEN_NUMBER,
	TOKEN_REGISTER,

	TOKEN_INSTRUCTION,
};

const char *token_type_to_string(const enum token_type type);

struct token {
	enum token_type type;

	union {
		enum instruction	ins;
		enum registers		reg;
		uint16_t		num;
		const char *		err;
	} as;

	const char *text;
	size_t textlen;

	size_t col;
	size_t line;
};
