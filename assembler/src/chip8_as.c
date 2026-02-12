#include "assembler.h"

#include <stdio.h>
#include <stdlib.h>

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

	if (assemble_source_file(path)) {
		fprintf(stderr, "invalid chip8 assembly\n");
		return 1;
	}

}
