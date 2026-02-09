CFLAGS = -Wall -Wpedantic -Wextra -std=c23 -g3 -fsanitize=address,leak,undefined

.PHONY: all clean

all: chip8

chip8: chip8.c

clean:
	rm -f ./chip8
