#pragma once

#include "token.h"
#include "lexer.h"
#include "common.h"
#include "instructions.h"

enum parse_status {
	PARSE_OK,
	PARSE_ERR,
	PARSE_EOF,
};

struct stmt {
	struct token instruction;

	size_t noperands;
	struct token operands[INSTRUCTION_OPERAND_MAX];
};

enum parse_status parse_statement(struct lexer *lexer, struct stmt *stmt);
