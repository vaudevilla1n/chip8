#pragma once

#include "parser.h"

#include <stdint.h>

enum translate_status {
	TRANSLATE_OK,
	TRANSLATE_ERR,
};

enum translate_status translate_statement_to_opcode(const struct stmt *stmt, uint16_t *op);
