#include "opcodes.h"

#include "common.h"
#include "instructions.h"

#include <stdarg.h>

static void assembler_error(const struct token *t, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "assembler: line %zu, line%zu: ", t->line, t->col);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");

	va_end(args);
}

static bool expect_operand_count(const struct stmt *stmt, const size_t expected)
{
	if (stmt->noperands == expected)
		return true;

	assembler_error(&stmt->instruction, "instruction requires %zu %s, got %zu %s",
		expected, (expected == 1) ? "operand" : "operands",
		stmt->noperands, (stmt->noperands == 1) ? "operand" : "operands" );

	return false;
}

static bool expect_register(const struct token *operand)
{
	if (operand->type == TOKEN_REGISTER)
		return true;
	
	assembler_error(operand, "expected register");
	
	return false;
}

static bool expect_number_literal(const struct token *operand)
{
	if (operand->type == TOKEN_NUMBER)
		return true;
	
	assembler_error(operand, "expected number literal");
	
	return false;
}

static bool expect_valid_address(const struct token *operand)
{
	if (!expect_number_literal(operand))
		return false;

	const uint16_t addr = operand->as.num;
	return (addr <= 0x0FFF);
}

uint16_t opcode_from_statement(const struct stmt *stmt)
{
	const enum instruction ins = stmt->instruction.as.ins;
	switch (ins) {
	case INS_SYS: {
		if (!expect_operand_count(stmt, 1))
			return OPCODE_INVALID;

		const struct token *operand = &stmt->operands[0];
		if (!expect_valid_address(operand))
			return OPCODE_INVALID;

		return (0x0000) & operand->as.num;
	} break;

	case INS_CLS: {
		if (!expect_operand_count(stmt, 0))
			return OPCODE_INVALID;

		return (0x00E0);
	} break;

	case INS_RET: {
		if (!expect_operand_count(stmt, 0))
			return OPCODE_INVALID;

		return (0x00EE);
	} break;

	case INS_JP: {
		if (!expect_operand_count(stmt, 1))
			return OPCODE_INVALID;

		const struct token *operand = &stmt->operands[0];
		if (!expect_valid_address(operand))
			return OPCODE_INVALID;

		return (0x1000) & operand->as.num;
	} break;

	case INS_CALL: {
		todo("INS_CALL");
	} break;

	case INS_SE: {
		todo("INS_SE");
	} break;

	case INS_SNE: {
		todo("INS_SNE");
	} break;

	case INS_LD: {
		todo("INS_LD");
	} break;

	case INS_ADD: {
		todo("INS_ADD");
	} break;

	case INS_SUB: {
		todo("INS_SUB");
	} break;

	case INS_SUBN: {
		todo("INS_SUBN");
	} break;

	case INS_OR: {
		todo("INS_OR");
	} break;

	case INS_XOR: {
		todo("INS_XOR");
	} break;

	case INS_AND: {
		todo("INS_AND");
	} break;

	case INS_SHR: {
		todo("INS_SHR");
	} break;

	case INS_SHL: {
		todo("INS_SHL");
	} break;

	case INS_RND: {
		todo("INS_RND");
	} break;

	case INS_SKP: {
		todo("INS_SKP");
	} break;

	case INS_SKNP: {
		todo("INS_SKNP");
	} break;

	default:	return OPCODE_INVALID;
	}
}
