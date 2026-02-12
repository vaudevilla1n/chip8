#pragma once

#include "lexer.h"
#include "token.h"
#include "instructions.h"

enum parse_status {
	PARSE_EOF,
	PARSE_ERR,
	PARSE_OK,
};

struct stmt {
	struct token instruction;

	size_t noperands;
	struct token operands[INSTRUCTION_OPERAND_MAX];
};

enum parse_status parse_statement(struct stmt *stmt, struct lexer *lexer);
