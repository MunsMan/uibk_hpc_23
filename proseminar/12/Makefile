MPICC=mpicc
CC_FLAGS=-O3 -std=gnu11 -Wall -Wextra -pedantic -lm

.PHONY: all
all: task1 task2

task1: task1.c
	@$(MPICC) $(CC_FLAGS) -o $@ $^

task2: task2.c
	@$(MPICC) $(CC_FLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f task1 task2 *_file*
