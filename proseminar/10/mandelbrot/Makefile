CC=gcc
MPICC=mpicc
CFLAGS=-std=gnu11 -Wall -Wextra -lm -O3 -ffast-math -march=native -g3

.PHONY: all
all: mandelbrot_seq mandelbrot_mpi

.PHONY: clean
clean:
	$(RM) mandelbrot_seq mandelbrot_seq.png mandelbrot_mpi mandelbrot_mpi.png

mandelbrot_seq: mandelbrot_seq.c
	$(CC) $(CFLAGS) -o $@ $^

mandelbrot_mpi: mandelbrot_mpi.c
	$(MPICC) $(CFLAGS) -o $@ $^
