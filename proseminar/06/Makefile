CC=gcc
CC_FLAGS=-O3 -std=gnu11 -Wall

.PHONY: all
all: nbody 

nbody: nbody.c
	@$(CC) $(CC_FLAGS) $< -o nbody

.PHONY: clean
clean:
	@rm nbody
