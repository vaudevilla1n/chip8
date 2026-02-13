#include "token.h"

#include <stdio.h>
#include <stdarg.h>

const char *token_type_to_string(const enum token_type type)
{
	switch (type) {
	case TOKEN_INVALID:	return "TOKEN_INVALID";

	case TOKEN_EOL:		return "TOKEN_EOL";
	case TOKEN_EOF:		return "TOKEN_EOF";

	case TOKEN_COMMA:	return "TOKEN_COMMA";
	case TOKEN_LBRACKET:	return "TOKEN_LBRACKET";
	case TOKEN_RBRACKET:	return "TOKEN_RBRACKET";

	case TOKEN_COMMENT:	return "TOKEN_COMMENT";
	
	case TOKEN_BCD:		return "TOKEN_BCD";
	case TOKEN_FONT:	return "TOKEN_FONT";

	case TOKEN_KEY:	return "TOKEN_KEY";

	case TOKEN_MEMORY:	return "TOKEN_MEMORY";

	case TOKEN_NUMBER:	return "TOKEN_NUMBER";
	case TOKEN_REGISTER:	return "TOKEN_REGISTER";
	case TOKEN_INSTRUCTION:	return "TOKEN_INSTRUCTION";

	default:	__builtin_unreachable();
	}
}

void token_error(const struct token *t, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "line %zu: col %zu: \'%.*s\': ", t->line, t->col, (int)t->textlen, t->text);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");

	va_end(args);
}
