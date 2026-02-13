#include "translate.h"

static inline bool is_addr(const struct token *op)
{
	return (op->type == TOKEN_NUMBER && op->as.num <= 0x0FFF);
}

static inline bool is_byte(const struct token *op)
{
	return (op->type == TOKEN_NUMBER && op->as.num <= 0xFF);
}

static inline bool is_reg(const struct token *op)
{
	return (op->type == TOKEN_REGISTER);
}

static inline bool is_byte_reg(const struct token *op)
{
	return (op->type == TOKEN_REGISTER && (REG_V0 <= op->as.reg && op->as.reg <= REG_VF));
}


static bool expect_operand_count(const struct stmt *stmt, const size_t expected)
{
	if (stmt->noperands == expected)
		return true;
	
	token_error(&stmt->instruction, "expected %zu operan%s; got %zu", expected, (expected == 1) ? "d" : "ds", stmt->noperands);
	return false;
}

static bool expect(const struct token *op, const bool cond, const char *msg)
{
	if (cond)
		return true;

	token_error(op, "invalid operand: %s", msg);
	return false;
}

static inline bool expect_addr(const struct token *op)
{
	return expect(op, is_addr(op), "expected an address");
}
static inline bool expect_byte(const struct token *op)
{
	return expect(op, is_byte(op), "expected a byte value");
}
static inline bool expect_reg(const struct token *op)
{
	return expect(op, is_reg(op), "expected a register");
}
static inline bool expect_byte_reg(const struct token *op)
{
	return expect(op, is_byte_reg(op), "expected a byte register");
}
static inline bool expect_nibble(const struct token *op)
{
	return expect(op, (op->type == TOKEN_NUMBER && op->as.num <= 0xF), "expected a byte register");
}

static enum translate_status translate_jp(const struct stmt *stmt, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];
	const struct token *op1 = &stmt->operands[1];

	if (stmt->noperands < 1 || stmt->noperands > 2) {
		token_error(&stmt->instruction, "expected 1 or 2 operands");
		return TRANSLATE_ERR;
	}

	if (stmt->noperands == 1) {
		if (!expect_addr(op0))
			return TRANSLATE_ERR;

		*opcode = (0x1000) | (op0->as.num);
	} else {
		if (!expect_byte_reg(op0) || !expect_addr(op1))
			return TRANSLATE_ERR;

		if (op0->as.reg != REG_V0) {
			token_error(op0, "only the V0 register is allowed here");
			return TRANSLATE_ERR;
		}

		*opcode = (0xB000) | (op0->as.num);
	}

	return TRANSLATE_OK;
}

static enum translate_status translate_add(const struct stmt *stmt, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];
	const struct token *op1 = &stmt->operands[1];

	if (!expect_operand_count(stmt, 2) || !expect_reg(op0))
		return TRANSLATE_ERR;

	if (is_byte_reg(op0) && is_byte(op1)) {
		*opcode = (0x7000) | (op0->as.reg << 8) | (op1->as.num);
	} else if (is_byte_reg(op0) && is_byte_reg(op1)) {
		*opcode = (0x8004) | (op0->as.reg << 8) | (op1->as.reg << 4);
	} else if (op0->as.reg == REG_I && is_byte_reg(op1)) {
		*opcode = (0xF01E) | (op1->as.reg << 8);
	} else {
		token_error(&stmt->instruction, "operand type mismatch");
		return TRANSLATE_ERR;
	}

	return TRANSLATE_OK;
}

static enum translate_status translate_arithmetic(const struct stmt *stmt, const uint16_t base_opcode, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];
	const struct token *op1 = &stmt->operands[1];

	if (!expect_operand_count(stmt, 2) || !expect_byte_reg(op0) || !expect_byte_reg(op1))
		return TRANSLATE_ERR;

	*opcode = (base_opcode) | (op0->as.reg << 8) | (op1->as.reg << 4);

	return TRANSLATE_OK;
}

static enum translate_status translate_shift(const struct stmt *stmt, const uint16_t base_opcode, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];

	if (!expect_operand_count(stmt, 1) || !expect_byte_reg(op0))
		return TRANSLATE_ERR;

	*opcode = (base_opcode) | (op0->as.reg << 8);

	return TRANSLATE_OK;
}

static enum translate_status translate_ld(const struct stmt *stmt, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];
	const struct token *op1 = &stmt->operands[1];

	if (!expect_operand_count(stmt, 2))
		return TRANSLATE_ERR;
	
	if (is_byte_reg(op0) && is_byte(op1)) {
		*opcode = (0x6000) | (op0->as.reg << 8) | (op1->as.num);
	} else if (is_byte_reg(op0) && is_byte_reg(op1)) {
		*opcode = (0x8000) | (op0->as.reg << 8) | (op1->as.reg << 4);
	} else if (is_reg(op0) && op0->as.reg == REG_I && is_addr(op1)) {
		*opcode = (0xA000) | (op1->as.num);
	} else if (is_byte_reg(op0) && is_reg(op1) && op1->as.reg == REG_DT) {
		*opcode = (0xF007) | (op0->as.reg << 8);
	} else if (is_byte_reg(op0) && op1->type == TOKEN_KEY) {
		*opcode = (0xF00A) | (op0->as.reg << 8);
	} else if (is_reg(op0) && op0->as.reg == REG_DT && is_byte_reg(op1)) {
		*opcode = (0xF015) | (op1->as.reg << 8);
	} else if (is_reg(op0) && op0->as.reg == REG_ST && is_byte_reg(op1)) {
		*opcode = (0xF018) | (op1->as.reg << 8);
	} else if (op0->type == TOKEN_FONT && is_byte_reg(op1)) {
		*opcode = (0xF029) | (op1->as.reg << 8);
	} else if (op0->type == TOKEN_BCD && is_byte_reg(op1)) {
		*opcode = (0xF033) | (op1->as.reg << 8);
	} else if (op0->type == TOKEN_MEMORY && is_byte_reg(op1)) {
		*opcode = (0xF055) | (op1->as.reg << 8);
	} else if (is_byte_reg(op0) && op1->type == TOKEN_MEMORY ) {
		*opcode = (0xF065) | (op0->as.reg << 8);
	} else {
		token_error(&stmt->instruction, "operand type mismatch");
		return TRANSLATE_ERR;
	}

	return TRANSLATE_OK;
}

enum translate_status translate_statement_to_opcode(const struct stmt *stmt, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];
	const struct token *op1 = &stmt->operands[1];
	const struct token *op2 = &stmt->operands[2];

	unused(op2);

	switch (stmt->instruction.as.ins) {
	case INS_SYS: {
		if (!expect_operand_count(stmt, 1) || !expect_addr(op0))
			return TRANSLATE_ERR;

		*opcode = (0x0000) | (op0->as.num);
	} break;


	case INS_CLS: {
		if (!expect_operand_count(stmt, 0))
			return TRANSLATE_ERR;

		*opcode = (0x00E0);
	} break;


	case INS_RET: {
		if (!expect_operand_count(stmt, 0))
			return TRANSLATE_ERR;

		*opcode = (0x00EE);
	} break;

	case INS_JP:	return translate_jp(stmt, opcode);

	case INS_CALL: {
		if (!expect_operand_count(stmt, 1) || !expect_addr(op0))
			return TRANSLATE_ERR;

		*opcode = (0x2000) | (op0->as.num);
	} break;


	case INS_SE: {
		if (!expect_operand_count(stmt, 2) || !expect_byte_reg(op0))
			return TRANSLATE_ERR;

		if (is_byte(op1)) {
			*opcode = (0x3000) | (op0->as.reg << 8) | (op1->as.num);
		} else if (is_byte_reg(op1)) {
			*opcode = (0x5000) | (op0->as.reg << 8) | (op1->as.reg << 4);
		} else {
			token_error(op1, "invalid operand: expected a byte value or register");
			return TRANSLATE_ERR;
		}
	} break;

	case INS_SNE: {
		if (!expect_operand_count(stmt, 2) || !expect_byte_reg(op0))
			return TRANSLATE_ERR;

		if (is_byte(op1)) {
			*opcode = (0x4000) | (op0->as.reg << 8) | (op1->as.num);
		} else if (is_byte_reg(op1)) {
			*opcode = (0x9000) | (op0->as.reg << 8) | (op1->as.reg << 4);
		} else {
			token_error(op1, "invalid operand: expected a byte value or register");
			return TRANSLATE_ERR;
		}
	} break;


	case INS_OR:	return translate_arithmetic(stmt, 0x8001, opcode);
	case INS_AND:	return translate_arithmetic(stmt, 0x8002, opcode);
	case INS_XOR:	return translate_arithmetic(stmt, 0x8003, opcode);

	case INS_ADD:	return translate_add(stmt, opcode);

	case INS_SUB:	return translate_arithmetic(stmt, 0x8005, opcode);
	case INS_SUBN:	return translate_arithmetic(stmt, 0x8007, opcode);

	case INS_SHR:	return translate_shift(stmt, 0x8006, opcode);
	case INS_SHL:	return translate_shift(stmt, 0x800E, opcode);


	case INS_RND: {
		if (!expect_operand_count(stmt, 2) || !expect_byte_reg(op0) || !expect_byte(op1))
			return TRANSLATE_ERR;

		*opcode = (0xC000) | (op0->as.reg << 8) | (op1->as.num);
	} break;


	case INS_DRW: {
		if (!expect_operand_count(stmt, 3)
				|| !expect_byte_reg(op0) || !expect_byte_reg(op1)
				|| !expect_nibble(op2))
			return TRANSLATE_ERR;

		*opcode = (0xD000) | (op0->as.reg << 8) | (op1->as.reg << 4) | (op2->as.num);
	} break;


	case INS_SKP: {
		if (!expect_operand_count(stmt, 1) || !expect_byte_reg(op0))
			return TRANSLATE_ERR;

		*opcode = (0xE09E) | (op0->as.reg << 8);
	} break;

	case INS_SKNP: {
		if (!expect_operand_count(stmt, 1) || !expect_byte_reg(op0))
			return TRANSLATE_ERR;

		*opcode = (0xE0A1) | (op0->as.reg << 8);
	} break;


	case INS_LD:	return translate_ld(stmt, opcode);


	default:	return TRANSLATE_ERR;
	}

	return TRANSLATE_OK;
}
