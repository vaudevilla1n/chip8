#pragma once

/*
	just experimenting with this
	idk if i will commit to this
*/

#include "parser.h"
#include "instructions.h"

#include <stdint.h>

#define TRANSLATION_INVALID	(0)

enum translation_status {
	TRANSLATION_OK,
	TRANSLATION_ERR
};

enum expected_operand {
	OPERAND_BCD		= 001,
	OPERAND_FONT		= 002,
	OPERAND_NUMBER		= 004,
	OPERAND_MEMORY		= 010,
	OPERAND_REGISTER	= 012,
};

struct translation_entry {
	uint16_t base_opcode;

	enum expected_operand operands[INSTRUCTION_OPERAND_MAX];

	size_t expected_operand_count;
};

extern struct translation_entry translation_table[TOTAL_INSTRUCTIONS];

enum translation_status translate_statement_to_opcode(const struct stmt *stmt);
