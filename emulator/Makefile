CFLAGS = -Wall -Wpedantic -Wextra -std=c23 -g3
LDFLAGS = -lcurses

SRC = $(wildcard src/*.c)
INC = $(wildcard src/*.h)

.PHONY: all clean

all: chip8

chip8: $(SRC) $(INC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o chip8

clean:
	rm -f ./chip8
