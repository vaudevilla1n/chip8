CFLAGS := -Wall -Wpedantic -Wextra -Wno-unused-function -std=c23 -g3 -fsanitize=address,leak,undefined

.PHONY: all clean

all: cdasm

cdasm: cdasm.c

clean:
	rm -f ./cdasm
