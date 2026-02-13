#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>

[[noreturn]] void usage(void)
{
	fprintf(stderr, "usage: ./chip8_as FILE\n");
	exit(1);
}

#define DEFAULT_ROM_PATH	"./a.out"

int main(int argc, char **argv)
{
	if (argc != 2)
		usage();
	
	const char *path = argv[1];

	if (assemble_source_file(path, DEFAULT_ROM_PATH)) {
		fprintf(stderr, "assembling unsuccessful :(\n");
		return 1;
	}

}
