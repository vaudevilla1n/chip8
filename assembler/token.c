#include "token.h"

const char *token_type_string(const enum token_type type)
{
	switch (type) {
	case TOKEN_EOF:		return "TOKEN_EOF";
	case TOKEN_INVALID:	return "TOKEN_INVALID";
	case TOKEN_COMMA:	return "TOKEN_COMMA";
	case TOKEN_NUMBER:	return "TOKEN_NUMBER";
	case TOKEN_REGISTER:	return "TOKEN_REGISTER";
	case TOKEN_INSTRUCTION:	return "TOKEN_INSTRUCTION";

	default:	__builtin_unreachable();
	}
}
