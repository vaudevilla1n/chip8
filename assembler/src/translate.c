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
