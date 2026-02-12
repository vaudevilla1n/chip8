#include "registers.h"

const char *registers_to_string(const enum registers reg)
{
	switch (reg) {
	case REG_V0:	return "V0";
	case REG_V1:	return "V1";
	case REG_V2:	return "V2";
	case REG_V3:	return "V3";
	case REG_V4:	return "V4";
	case REG_V5:	return "V5";
	case REG_V6:	return "V6";
	case REG_V7:	return "V7";
	case REG_V8:	return "V8";
	case REG_V9:	return "V9";
	case REG_VA:	return "VA";
	case REG_VB:	return "VB";
	case REG_VC:	return "VC";
	case REG_VD:	return "VD";
	case REG_VE:	return "VE";
	case REG_VF:	return "VF";

	case REG_I:	return "I";

	default:	__builtin_unreachable();
	}
}

enum registers register_lookup(const char *id, const size_t len)
{
	if (len == 1 && id[0] == 'I')
		return REG_I;
	
	if (len != 2)
		return REG_INVALID;

	if (id[0] == 'V') {
		if ('0' <= id[1] && id[1] <= '9')
			return id[1] - '0';
		if ('A' <= id[1] && id[1] <= 'F')
			return id[1] - 'A' + 10;
	} else if (id[0] == 'S' && id[1] == 'T') {
		return REG_ST;
	} else if (id[0] == 'D' && id[1] == 'T') {
		return REG_DT;
	}

	return REG_INVALID;
}
