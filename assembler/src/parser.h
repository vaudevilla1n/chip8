#pragma once

#include "lexer.h"
#include "token.h"

enum parse_status {
	PARSE_EOF,
	PARSE_ERROR,
	PARSE_SUCCESS,
};

#define PARSE_OPERANDS_MAX	3

struct stmt {
	struct token instruction;

	size_t noperands;
	struct token operands[PARSE_OPERANDS_MAX];
};

enum parse_status parse_statement(struct stmt *stmt, struct lexer *lexer);
