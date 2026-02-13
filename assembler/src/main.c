#include "common.h"
#include "assembler.h"

#define _XOPEN_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

[[noreturn]] void usage(void)
{
	fprintf(stderr, "usage: ./casm FILE\n");
	exit(1);
}

#define DEFAULT_ROM_PATH	"./a.out"

static int parse_arguments(const int argc, char **argv, const char **rom_path, const char **src_path)
{
	switch (argc) {
	case 2: {
		*src_path = argv[1];
		return 0;
	}

	case 4: {
		if (STREQ(argv[1], "-o")) {
			*rom_path = argv[2];
			*src_path = argv[3];
		} else if (STREQ(argv[2], "-o")) {
			*src_path = argv[1];
			*rom_path = argv[3];
		} else {
			return 1;
		}

		return 0;
	}

	default:	return 1;
	}
}

int main(int argc, char **argv)
{
	const char *rom_path = DEFAULT_ROM_PATH;
	const char *src_path = nullptr;

	if (parse_arguments(argc, argv, &rom_path, &src_path))
		usage();

	if (assemble_source_file(src_path, rom_path)) {
		fprintf(stderr, "assembling unsuccessful :(\n");
		return 1;
	}

}
