#include "instructions.h"

#include "common.h"

#define INSTRUCTION_OPERAND_MAX	3

static const char *instruction_identifiers[TOTAL_INSTRUCTIONS] = {
	[INS_SYS] = "sys",
	[INS_CLS] = "cls",
	[INS_RET] = "ret",
	[INS_JP] = "jp",
	[INS_CALL] = "call",
	[INS_SE] = "se",
	[INS_SNE] = "sne",
	[INS_LD] = "ld",
	[INS_ADD] = "add",
	[INS_SUB] = "sub",
	[INS_SUBN] = "subn",
	[INS_OR] = "or",
	[INS_XOR] = "xor",
	[INS_AND] = "and",
	[INS_SHR] = "shr",
	[INS_SHL] = "shl",
	[INS_RND] = "rnd",
	[INS_DRW] = "drw",
	[INS_SKP] = "skp",
	[INS_SKNP] = "sknp",
};

const char *instruction_to_string(const enum instruction ins)
{
	if (ins >= TOTAL_INSTRUCTIONS)
		return "INS_INVALID";
	
	return instruction_identifiers[ins];
}

