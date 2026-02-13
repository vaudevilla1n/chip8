#include "translate.h"

/*
	A -> memory address operand
	V -> byte register operand
	B -> byte value operand
	I -> I register operand
	M -> memory ([I]) operand
	S -> ST register operand
	D -> DT register operand
	K -> pressed key operand
	F -> font operand
	BCD -> bcd operand
*/
enum base_opcode {
	OP_SYS		= 0x0000,
	OP_CLS		= 0x00E0,
	OP_RET		= 0x00EE,

	OP_JP_A		= 0x1000,
	OP_JP_VA	= 0xB000,

	OP_CALL		= 0x2000,

	OP_SE_VB	= 0x3000,
	OP_SE_VV	= 0x5000,

	OP_SNE_VB	= 0x4000,
	OP_SNE_VV	= 0x9000,

	OP_OR		= 0x8001,
	OP_AND		= 0x8002,
	OP_XOR		= 0x8003,
	OP_SUB		= 0x8005,
	OP_SHR		= 0x8006,
	OP_SUBN		= 0x8007,
	OP_SHL		= 0x800E,

	OP_RND		= 0xC000,

	OP_DRW		= 0xD000,

	OP_SKP		= 0xE09E,
	OP_SKNP		= 0xE0A1,

	OP_ADD_VB	= 0x7000,
	OP_ADD_VV	= 0x8004,
	OP_ADD_IV	= 0xF01E,

	OP_LD_VB	= 0x6000,
	OP_LD_VV	= 0x8000,
	OP_LD_IA	= 0xA000,
	OP_LD_VD	= 0xF007,
	OP_LD_VK	= 0xF00A,
	OP_LD_DV	= 0xF015,
	OP_LD_SV	= 0xF018,
	OP_LD_FV	= 0xF029,
	OP_LD_BCDV	= 0xF033,
	OP_LD_MV	= 0xF055,
	OP_LD_VM	= 0xF065,
};

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

		*opcode = (OP_JP_A) | (op0->as.num);
	} else {
		if (!expect_byte_reg(op0) || !expect_addr(op1))
			return TRANSLATE_ERR;

		if (op0->as.reg != REG_V0) {
			token_error(op0, "only the V0 register is allowed here");
			return TRANSLATE_ERR;
		}

		*opcode = (OP_JP_VA) | (op0->as.num);
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
		*opcode = (OP_ADD_VB) | (op0->as.reg << 8) | (op1->as.num);
	} else if (is_byte_reg(op0) && is_byte_reg(op1)) {
		*opcode = (OP_ADD_VV) | (op0->as.reg << 8) | (op1->as.reg << 4);
	} else if (op0->as.reg == REG_I && is_byte_reg(op1)) {
		*opcode = (OP_ADD_IV) | (op1->as.reg << 8);
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
		*opcode = (OP_LD_VB) | (op0->as.reg << 8) | (op1->as.num);
	} else if (is_byte_reg(op0) && is_byte_reg(op1)) {
		*opcode = (OP_LD_VV) | (op0->as.reg << 8) | (op1->as.reg << 4);
	} else if (is_reg(op0) && op0->as.reg == REG_I && is_addr(op1)) {
		*opcode = (OP_LD_IA) | (op1->as.num);
	} else if (is_byte_reg(op0) && is_reg(op1) && op1->as.reg == REG_DT) {
		*opcode = (OP_LD_VD) | (op0->as.reg << 8);
	} else if (is_byte_reg(op0) && op1->type == TOKEN_KEY) {
		*opcode = (OP_LD_VK) | (op0->as.reg << 8);
	} else if (is_reg(op0) && op0->as.reg == REG_DT && is_byte_reg(op1)) {
		*opcode = (OP_LD_DV) | (op1->as.reg << 8);
	} else if (is_reg(op0) && op0->as.reg == REG_ST && is_byte_reg(op1)) {
		*opcode = (OP_LD_SV) | (op1->as.reg << 8);
	} else if (op0->type == TOKEN_FONT && is_byte_reg(op1)) {
		*opcode = (OP_LD_FV) | (op1->as.reg << 8);
	} else if (op0->type == TOKEN_BCD && is_byte_reg(op1)) {
		*opcode = (OP_LD_BCDV) | (op1->as.reg << 8);
	} else if (op0->type == TOKEN_MEMORY && is_byte_reg(op1)) {
		*opcode = (OP_LD_MV) | (op1->as.reg << 8);
	} else if (is_byte_reg(op0) && op1->type == TOKEN_MEMORY ) {
		*opcode = (OP_LD_VM) | (op0->as.reg << 8);
	} else {
		token_error(&stmt->instruction, "operand type mismatch");
		return TRANSLATE_ERR;
	}

	return TRANSLATE_OK;
}

static enum translate_status translate_basic_instruction(const struct stmt *stmt, const uint16_t base_opcode, uint16_t *opcode)
{
	if (!expect_operand_count(stmt, 0))
		return TRANSLATE_ERR;

	*opcode = (base_opcode);
	
	return TRANSLATE_OK;
}

static enum translate_status translate_skip(const struct stmt *stmt, const uint16_t base_opcode_vb, const uint16_t base_opcode_vv, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];
	const struct token *op1 = &stmt->operands[1];

	if (!expect_operand_count(stmt, 2) || !expect_byte_reg(op0))
		return TRANSLATE_ERR;

	if (is_byte(op1)) {
		*opcode = (base_opcode_vb) | (op0->as.reg << 8) | (op1->as.num);
	} else if (is_byte_reg(op1)) {
		*opcode = (base_opcode_vv) | (op0->as.reg << 8) | (op1->as.reg << 4);
	} else {
		token_error(op1, "invalid operand: expected a byte value or register");
		return TRANSLATE_ERR;
	}

	return TRANSLATE_OK;
}

static enum translate_status translate_skip_pressed(const struct stmt *stmt, const uint16_t base_opcode, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];

	if (!expect_operand_count(stmt, 1) || !expect_byte_reg(op0))
		return TRANSLATE_ERR;

	*opcode = (base_opcode) | (op0->as.reg << 8);

	return TRANSLATE_OK;
}

static enum translate_status translate_jump(const struct stmt *stmt, const uint16_t base_opcode, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];

	if (!expect_operand_count(stmt, 1) || !expect_addr(op0))
		return TRANSLATE_ERR;

	*opcode = (base_opcode) | (op0->as.num);

	return TRANSLATE_OK;
}

static enum translate_status translate_rnd(const struct stmt *stmt, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];
	const struct token *op1 = &stmt->operands[1];

	if (!expect_operand_count(stmt, 2) || !expect_byte_reg(op0) || !expect_byte(op1))
		return TRANSLATE_ERR;

	*opcode = (OP_RND) | (op0->as.reg << 8) | (op1->as.num);

	return TRANSLATE_OK;
}

static enum translate_status translate_drw(const struct stmt *stmt, uint16_t *opcode)
{
	const struct token *op0 = &stmt->operands[0];
	const struct token *op1 = &stmt->operands[1];
	const struct token *op2 = &stmt->operands[2];

	if (!expect_operand_count(stmt, 3)
			|| !expect_byte_reg(op0) || !expect_byte_reg(op1)
			|| !expect_nibble(op2))
		return TRANSLATE_ERR;

	*opcode = (OP_DRW) | (op0->as.reg << 8) | (op1->as.reg << 4) | (op2->as.num);

	return TRANSLATE_OK;
}

enum translate_status translate_statement_to_opcode(const struct stmt *stmt, uint16_t *opcode)
{
	switch (stmt->instruction.as.ins) {
	case INS_SYS:	return translate_jump(stmt, OP_SYS, opcode);
	case INS_CALL:	return translate_jump(stmt, OP_CALL, opcode);

	case INS_JP:	return translate_jp(stmt, opcode);

	case INS_CLS:	return translate_basic_instruction(stmt, OP_CLS, opcode);
	case INS_RET:	return translate_basic_instruction(stmt, OP_RET, opcode);

	case INS_ADD:	return translate_add(stmt, opcode);

	case INS_OR:	return translate_arithmetic(stmt, OP_OR, opcode);
	case INS_AND:	return translate_arithmetic(stmt, OP_AND, opcode);
	case INS_XOR:	return translate_arithmetic(stmt, OP_OR, opcode);
	case INS_SUB:	return translate_arithmetic(stmt, OP_SUB, opcode);
	case INS_SUBN:	return translate_arithmetic(stmt, OP_SUBN, opcode);

	case INS_SHR:	return translate_shift(stmt, OP_SHR, opcode);
	case INS_SHL:	return translate_shift(stmt, OP_SHL, opcode);

	case INS_RND:	return translate_rnd(stmt, opcode);

	case INS_DRW:	return translate_drw(stmt, opcode);

	case INS_SE:	return translate_skip(stmt, OP_SE_VB, OP_SE_VV, opcode);
	case INS_SNE:	return translate_skip(stmt, OP_SNE_VB, OP_SNE_VV, opcode);

	case INS_SKP:	return translate_skip_pressed(stmt, OP_SKP, opcode);
	case INS_SKNP:	return translate_skip_pressed(stmt, OP_SKNP, opcode);

	case INS_LD:	return translate_ld(stmt, opcode);

	default:	return TRANSLATE_ERR;
	}
}
