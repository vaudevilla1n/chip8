CFLAGS := -Wall -Wpedantic -Wextra -std=c23 -g3 -fsanitize=address,leak,undefined

SRC	:= $(wildcard src/*.c)
INC	:= $(wildcard src/*.h)

.PHONY: all clean

all: chip8_as

chip8_as: $(SRC) $(INC)
	$(CC) $(CFLAGS) $(SRC) -o chip8_as

clean:
	rm -f ./chip8_as
