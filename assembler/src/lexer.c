#include "lexer.h"

#include "common.h"
#include "registers.h"
#include "instructions.h"

#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#define INSTRUCTION_TABLE_CAPACITY		(TOTAL_INSTRUCTIONS)
#define INSTRUCTION_TABLE_COLLISION_MAX		(TOTAL_INSTRUCTIONS)

struct instruction_lookup_entry {
	const char *key;
	size_t keylen;
	enum instruction val;
};

static struct instruction_lookup_entry instruction_lookup_table[INSTRUCTION_TABLE_CAPACITY][INSTRUCTION_TABLE_COLLISION_MAX];

static uint64_t hash(const char *key)
{
	uint64_t h = 805306457;

	h ^= (uint64_t)key[0] << 16;
	h ^= (uint64_t)key[1];

	return h;
}

static void instruction_table_insert(const char *key, const enum instruction val)
{
	const uint64_t h = hash(key);
	const size_t i = h % INSTRUCTION_TABLE_CAPACITY;
	
	struct instruction_lookup_entry e = {
		.key = key,
		.keylen = strlen(key),
		.val = val,
	};

	size_t j;
	for (j = 0; instruction_lookup_table[i][j].key; j++)
		;
	
	instruction_lookup_table[i][j] = e;
}

static void instruction_lookup_table_init(void)
{
	for (enum instruction i = 0; i < TOTAL_INSTRUCTIONS; i++) {
		const char *key = instruction_to_string(i);

		instruction_table_insert(key, i);
	}
}

static enum instruction lookup_instruction(const char *id, const size_t len)
{
	/*
		iteration + strncmp 

		real	0m0.008s
		user	0m0.003s
		sys	0m0.004s
	 */

	if (len < 2)
		return INS_INVALID;

	const uint64_t h = hash(id);
	const size_t i = h % INSTRUCTION_TABLE_CAPACITY;

	for (size_t j = 0; instruction_lookup_table[i][j].key; j++) {
		struct instruction_lookup_entry *e = &instruction_lookup_table[i][j];

		if (!strncmp(id, e->key, e->keylen))
			return e->val;
	}
	
	return INS_INVALID;
}


static enum registers lookup_register(const char *id, const size_t len)
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


static char peek_char(const struct lexer *lexer)
{
	return (lexer->pos < lexer->srclen) ? lexer->src[lexer->pos] : '\0';
}

static char next_char(struct lexer *lexer)
{
	if (lexer->pos >= lexer->srclen)
		return '\0';

	const char c = lexer->src[lexer->pos++];

	if (c == '\n') {
		lexer->col = 1;
		lexer->line++;
	} else {
		lexer->col++;
	}

	return c;
}

static void invalid_token(struct token *t, const char *err)
{
	t->type = TOKEN_INVALID;
	t->as.err = err;
}


static inline bool ishexdigit(const char d)
{
	return (isdigit(d) || ('a' <= d && d <= 'f') || ('A' <= d && d <= 'F'));
}

static inline bool isoctdigit(const char d)
{
	return ('0' <= d && d <= '7');
}

static void lex_number(struct lexer *lexer, struct token *t, const size_t start, const char init)
{
	if (init == '0' && (peek_char(lexer) == 'x' || peek_char(lexer) == 'X')) {
		next_char(lexer);

		if (!ishexdigit(peek_char(lexer))) {
			invalid_token(t, "erroneous hexadecimal delimiter");
			return;
		}

		while (ishexdigit(peek_char(lexer)))
			next_char(lexer);
	} else if (init == '0') {
		if (!isoctdigit(peek_char(lexer))) {
			invalid_token(t, "erroneous octal delimiter");
			return;
		}

		while (isoctdigit(peek_char(lexer)))
			next_char(lexer);
	} else {
		while (isdigit(peek_char(lexer)))
			next_char(lexer);
	}

	t->textlen = lexer->pos - start;

	const uint32_t x = strtoul(t->text, nullptr, 0);

	if (errno == ERANGE || x > USHRT_MAX) {
		invalid_token(t, "number literal out of range");
	} else {
		t->type = TOKEN_NUMBER;
		t->as.num = (uint16_t)x;
	}
}


static bool set_register_token(struct token *t)
{
	const enum registers reg = lookup_register(t->text, t->textlen);

	if (reg == REG_INVALID)
		return false;
	
	t->type = TOKEN_REGISTER;
	t->as.reg = reg;

	return true;
}

static bool set_instruction_token(struct token *t)
{
	const enum instruction ins = lookup_instruction(t->text, t->textlen);

	if (ins == INS_INVALID)
		return false;
	
	t->type = TOKEN_INSTRUCTION;
	t->as.ins = ins;

	return true;
}

static void lex_identifier(struct lexer *lexer, struct token *t, const size_t start)
{
	while (isalnum(peek_char(lexer)))
		next_char(lexer);

	t->textlen = lexer->pos - start;
	
	if (set_register_token(t))
		return;

	if (set_instruction_token(t))
		return;

	invalid_token(t, "unknown identifier");
}


static void lex_memory(struct lexer *lexer, struct token *t)
{
	if (peek_char(lexer) != 'I') {
		invalid_token(t, "invalid register: only the I register can be used here");
		return;
	}
	next_char(lexer);

	if (peek_char(lexer) != ']') {
		invalid_token(t, "missing closing bracket");
		return;
	}
	next_char(lexer);

	t->type = TOKEN_MEMORY;
}


static void lex_comment(struct lexer *lexer, struct token *t)
{
	for (;;) {
		const char c = peek_char(lexer);

		if (!c || c == '\n')
			break;

		next_char(lexer);
	}

	t->type = TOKEN_COMMENT;
}


static void lex_eof(struct token *t)
{
	t->type = TOKEN_EOF;
	t->text = "EOF";
	t->textlen = 3;
}


static void lex_eol(struct token *t)
{
	t->type = TOKEN_EOL;
	t->text = "EOL";
	t->textlen = 3;
}


static void skip_whitespace(struct lexer *lexer)
{
	for (;;) {
		const char c = peek_char(lexer);

		if (!isspace(c) || c == '\n')
			break;

		next_char(lexer);
	}
}

static void lexer_advance(struct lexer *lexer)
{
	skip_whitespace(lexer);
	
	struct token t = {
		.text = lexer->src + lexer->pos,

		.col = lexer->col,
		.line = lexer->line,
	};

	const size_t start = lexer->pos;

	const char init = next_char(lexer);
	switch (init) {
	case '\0':	lex_eof(&t); break;

	case '\n':	lex_eol(&t); break;

	case ',':	t.type = TOKEN_COMMA; break;

	case 'B':	t.type = TOKEN_BCD; break;

	case 'F':	t.type = TOKEN_FONT; break;

	case 'K':	t.type = TOKEN_KEY; break;

	case '[':	lex_memory(lexer, &t); break;

	case ';':	lex_comment(lexer, &t); break;

	default: {
		if (isdigit(init))
			lex_number(lexer, &t, start, init);
		else if (isalpha(init))
			lex_identifier(lexer, &t, start);
		else
			invalid_token(&t, "unknown token");
	} break;
	}

	if (!t.textlen)
		t.textlen = lexer->pos - start;
	
	lexer->token = t;
}

struct lexer lexer_new(const char *src, const size_t srclen)
{
	instruction_lookup_table_init();

	struct lexer lexer = {
		.src = src,
		.srclen = srclen,

		.line = 1,
		.col = 1,
	};

	lexer_advance(&lexer);

	return lexer;
}

struct token lexer_next(struct lexer *lexer)
{
	const struct token ret = lexer->token;

	lexer_advance(lexer);

	return ret;
}

struct token lexer_peek(struct lexer *lexer)
{
	return lexer->token;
}

