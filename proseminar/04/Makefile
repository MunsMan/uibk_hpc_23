CC=gcc
MPICC=mpicc
CC_FLAGS=-O3 -std=gnu11 -Wall -Wextra -pedantic -lm
BUILD=build/

pre:
	mkdir -p build



.PHONY: all
all: pre heat_stencil_1D_mpi heat_stencil_2D_mpi_old heat_stencil_2D_mpi_opt heat_stencil_2D_seq


heat_stencil_2D_seq: heat_stencil_2D_seq.c
	@$(CC) $(CC_FLAGS) $< -o $(BUILD)heat_stencil_2D_seq

heat_stencil_2D_mpi_old: heat_stencil_2D_mpi_old.c
	@$(MPICC) $(CC_FLAGS) $< -o $(BUILD)heat_stencil_2D_mpi_old

heat_stencil_2D_mpi_opt: heat_stencil_2D_mpi_opt.c
	@$(MPICC) $(CC_FLAGS) $< -o $(BUILD)heat_stencil_2D_mpi_opt

heat_stencil_1D_mpi: heat_stencil_1D_mpi.c
	@$(MPICC) $(CC_FLAGS) $< -o $(BUILD)heat_stencil_1D_mpi

.PHONY: clean
clean:
	rm -r $(BUILD)

