#pragma once

#include "lexer.h"
#include "token.h"

enum parse_status {
	PARSE_EOF,
	PARSE_ERROR,
	PARSE_SUCCESS,
};

enum operand_type {
	OPERAND_REGISTER,
	OPERAND_NUMBER,
	OPERAND_FONT,
	OPERAND_BCD,
	OPERAND_MEMORY,
};

#define PARSE_OPERANDS_MAX	3

struct operand {
	enum operand_type type;
	struct token val;
};

struct stmt {
	struct token instruction;

	size_t noperands;
	struct operand operands[PARSE_OPERANDS_MAX];
};

enum parse_status parse_statement(struct stmt *stmt, struct lexer *lexer);
