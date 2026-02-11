#include "instructions.h"

#include "common.h"

#include <string.h>
#include <stdint.h>

static const char *instruction_identifiers[TOTAL_INSTRUCTIONS] = {
	[INS_SYS] = "sys",
	[INS_CLS] = "cls",
	[INS_RET] = "ret",
	[INS_JP] = "jp",
	[INS_CALL] = "call",
	[INS_SE] = "se",
	[INS_SNE] = "sne",
	[INS_LD] = "ld",
	[INS_ADD] = "add",
	[INS_SUB] = "sub",
	[INS_SUBN] = "subn",
	[INS_OR] = "or",
	[INS_XOR] = "xor",
	[INS_AND] = "and",
	[INS_SHR] = "shr",
	[INS_SHL] = "shl",
	[INS_RND] = "rnd",
	[INS_SKP] = "skp",
	[INS_SKNP] = "sknp",
};

const char *instruction_to_string(const enum instruction ins)
{
	if (ins >= TOTAL_INSTRUCTIONS)
		return "INS_INVALID";
	
	return instruction_identifiers[ins];
}

#define TABLE_CAPACITY		(TOTAL_INSTRUCTIONS)
#define TABLE_COLLISION_MAX	(TOTAL_INSTRUCTIONS)

struct table_entry {
	const char *key;
	enum instruction val;
};

static struct table_entry instruction_lookup_table[TABLE_CAPACITY][TABLE_COLLISION_MAX];

static uint64_t hash(const char *key)
{
	uint64_t h = 805306457;

	h ^= (uint64_t)key[0] << 16;
	h ^= (uint64_t)key[1];

	return h;
}

static void table_insert(const char *key, const enum instruction val)
{
	const uint64_t h = hash(key);
	const size_t i = h % TABLE_CAPACITY;
	
	struct table_entry e = {
		.key = key,
		.val = val,
	};

	size_t j;
	for (j = 0; instruction_lookup_table[i][j].key; j++)
		;
	
	instruction_lookup_table[i][j] = e;
}

void instruction_lookup_table_init(void)
{
	for (enum instruction i = 0; i < TOTAL_INSTRUCTIONS; i++) {
		const char *key = instruction_identifiers[i];

		table_insert(key, i);
	}
}

enum instruction instruction_lookup(const char *id, const size_t len)
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
	const size_t i = h % TABLE_CAPACITY;

	for (size_t j = 0; instruction_lookup_table[i][j].key; j++) {
		struct table_entry *e = &instruction_lookup_table[i][j];

		if (!strncmp(id, e->key, len))
			return e->val;
	}
	
	return INS_INVALID;
}
