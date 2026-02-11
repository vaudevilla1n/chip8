#pragma once

#include <stddef.h>

enum registers {
	REG_V0, REG_V1, REG_V2, REG_V3,
	REG_V4, REG_V5, REG_V6, REG_V7,
	REG_V8, REG_V9, REG_VA, REG_VB,
	REG_VC, REG_VD, REG_VE, REG_VF,

	TOTAL_REGISTERS,

	REG_INVALID,
};

const char *registers_to_string(const enum registers reg);

enum registers register_lookup(const char *id, const size_t len);
