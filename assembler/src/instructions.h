#pragma once

#include <stddef.h>

enum instruction {
	INS_SYS,

	INS_CLS,

	INS_RET, INS_JP, INS_CALL,

	INS_SE, INS_SNE,

	INS_LD,

	INS_ADD, INS_SUB, INS_SUBN,
	INS_OR, INS_XOR, INS_AND, INS_SHR, INS_SHL,

	INS_RND,

	INS_DRW,

	INS_SKP, INS_SKNP,

	TOTAL_INSTRUCTIONS,

	INS_INVALID,
};

const char *instruction_to_string(const enum instruction ins);

void instruction_lookup_table_init(void);
enum instruction instruction_lookup(const char *id, const size_t len);
