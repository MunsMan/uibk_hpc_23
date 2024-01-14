MPICC=mpicc
CC_FLAGS=-O3 -std=gnu11 -Wall -Wextra -pedantic -lm

.PHONY: all
all: individual single-file-individual-fp

individual: individual.c
	@$(MPICC) $(CC_FLAGS) -o $@ $^

single-file-individual-fp: single-file-individual-fp.c
	@$(MPICC) $(CC_FLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f individual *file.txt single-file-individual-fp
