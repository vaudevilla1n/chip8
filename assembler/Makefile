CFLAGS = -Wall -Wpedantic -Wextra -std=c23 -g3 -fsanitize=address,leak,undefined

.PHONY: all clean

all: chip8_as

chip8_as: chip8_as.c lexer.c lexer.h token.c token.h common.h
	$(CC) $(CFLAGS) chip8_as.c lexer.c token.c -o chip8_as

clean:
	rm -f ./chip8_as
