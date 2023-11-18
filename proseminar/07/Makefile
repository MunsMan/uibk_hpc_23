MPICC=mpicc
CC_FLAGS=-O3 -std=gnu11 -Wall -Wextra -pedantic -lm

nbody_octree: nbody_octree.c
	@$(MPICC) $(CC_FLAGS) -o $@ $^
