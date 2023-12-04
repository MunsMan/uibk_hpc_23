CC=gcc
MPICC=mpicc
CC_FLAGS=-O3 -std=gnu11 -Wall

.PHONY: all
all: pi_mpi

pi_mpi: pi_mpi.c
	@$(MPICC) $(CC_FLAGS) $< -o pi_mpi

.PHONY: clean
clean:
	@rm pi_mpi

.PHONY: run
run: all
	@echo "Parallel:"
	@mpiexec ./pi_mpi $(ARGS)
	@echo
