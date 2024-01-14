MPICC=mpicc
CC_FLAGS=-O3 -std=gnu11 -Wall -Wextra -pedantic -lm

individual: individual.c
	@$(MPICC) $(CC_FLAGS) -o $@ $^

clean:
	rm -f individual *file.txt
