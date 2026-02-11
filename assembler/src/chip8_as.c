#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "instructions.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

struct file {
	char *dat;
	size_t len;
};

struct file read_file(const char *path);
void close_file(struct file *f);


[[noreturn]] void usage(void)
{
	fprintf(stderr, "usage: ./chip8_as FILE\n");
	exit(1);
}

int main(int argc, char **argv)
{
	if (argc != 2)
		usage();
	
	const char *path = argv[1];

	struct file src = read_file(path);

	if (!src.dat) {
		perror(path);
		return 1;
	}

	instruction_lookup_table_init();

	struct lexer lexer = lexer_new(src.dat, src.len);

	/*
	for (;;) {
		const struct token t = lexer_next(&lexer);

		if (t.type == TOKEN_EOF)
			break;

		printf("%-20s \'%.*s\' line %zu, col %zu", token_type_to_string(t.type), (int)t.textlen, t.text, t.line, t.col);

		if (t.type == TOKEN_INVALID)
			printf(" (%s)", t.as.err);

		printf("\n");
	}
	*/

	struct stmt stmt;
	while (!parse_stmt(&stmt, &lexer)) {
		printf("%-8s", instruction_to_string(stmt.instruction.as.ins));
		
		for (size_t i = 0; i < stmt.noperands; i++) {
			if (i == 1)
				printf(", ");

			struct token *operand = stmt.operands + i;

			switch (operand->type) {
			case TOKEN_REGISTER:	printf("%s", registers_to_string(operand->as.reg)); break;
			case TOKEN_NUMBER:	printf("0x%hx", operand->as.num); break;

			default:	__builtin_unreachable();
			}
		}

		printf("\n");
	}

	close_file(&src);
}

struct file read_file(const char *path)
{
	struct file f;

	struct stat stats;

	if (stat(path, &stats))
		return (struct file){ 0 };
	
	const int fd = open(path, O_RDONLY);
	if (fd == -1)
		return (struct file){ 0 };
	
	f.len = stats.st_size;
	f.dat = mmap(nullptr, f.len, PROT_READ, MAP_PRIVATE, fd, 0);

	close(fd);

	if (f.dat == MAP_FAILED)
		return (struct file){ 0 };
	
	return f;
}

void close_file(struct file *f)
{
	munmap(f->dat, f->len);
}
