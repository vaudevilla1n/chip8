#include "translate.h"


struct translation_entry translation_table[TOTAL_INSTRUCTIONS] = {
	[INS_SYS] = {
		.base_opcode = 0x0000,
		.operands = {
			[0] = OPERAND_NUMBER,
		},
		.expected_operand_count = 1,
	},
	[INS_RET] = {
		.base_opcode = 0x00EE,
		.expected_operand_count = 0,
	}
};

static bool expect_operand_count(const struct stmt *stmt, const size_t expected)
{
	if (stmt->noperands == expected)
		return true;
	
	token_error(&stmt->instruction, "expected %zu operan%s; got %zu", expected, (expected == 1) ? "d" : "ds", stmt->noperands);
	return false;
}

static bool expect_address_operand(const struct token *operand)
{
	if (operand->type == TOKEN_NUMBER && operand->as.num <= 0x0FFF)
		return true;
	
	token_error(&operand, "invalid operand: expected an address");
	return false;
}

enum translation_status translate_statement_to_opcode(const struct stmt *stmt)
{
	switch (stmt->instruction->as.ins) {
	case INS_SYS: {
		if (!expect_operand_count(stmt, 1) || expect_address_operand(&stmt->operands[0]));
			return TRANSLATION_ERR;

		return (0x0000) | (stmt->operands[0].as.num);
	} break;
	}

	return TRANSLATION_OK;
}
