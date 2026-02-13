CFLAGS := -Wall -Wpedantic -Wextra -std=c23 -g3 -fsanitize=address,leak,undefined

SRC	:= $(wildcard src/*.c)
INC	:= $(wildcard src/*.h)

.PHONY: all clean

all: casm

casm: $(SRC) $(INC)
	$(CC) $(CFLAGS) $(SRC) -o casm

clean:
	rm -f ./casm
