CFLAGS := -Wall -Wpedantic -Wextra -Wno-unused-function -std=c23 -g3 -fsanitize=address,leak,undefined

SRC	:= $(wildcard src/*.c)
INC	:= $(wildcard src/*.h)

.PHONY: all debug clean

all: casm

debug: CFLAGS += -DDEBUG
debug: casm

casm: $(SRC) $(INC)
	$(CC) $(CFLAGS) $(SRC) -o casm


clean:
	rm -f ./casm
