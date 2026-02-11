#include "token.h"
#include "lexer.h"

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

	struct lexer lexer = lexer_new(src.dat, src.len);

	for (;;) {
		const struct token t = lexer_next(&lexer);

		if (t.type == TOKEN_EOF)
			break;

		printf("%s \'%.*s\' line %zu, col %zu\n", token_type_string(t.type), (int)t.textlen, t.text, t.line, t.col);
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
	f.dat = mmap(nullptr, f.len, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);

	close(fd);

	if (f.dat == MAP_FAILED)
		return (struct file){ 0 };
	
	return f;
}

void close_file(struct file *f)
{
	munmap(f->dat, f->len);
}
