#pragma once

#include "lexer.h"
#include "token.h"

#define STATEMENT_MAX_OPERANDS	2

struct stmt {
	struct token instruction;

	struct token operands[STATEMENT_MAX_OPERANDS];
	size_t noperands;
};

int parse_stmt(struct stmt *stmt, struct lexer *lexer);
