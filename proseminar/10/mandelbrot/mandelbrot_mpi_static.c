#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// Include that allows to print result as an image
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define DEFAULT_SIZE_X 1280
#define DEFAULT_SIZE_Y 720

// RGB image will hold 3 color channels
#define NUM_CHANNELS 3
// max iterations cutoff
#define MAX_ITER 10000

#define IND(Y, X, SIZE_Y, SIZE_X, CHANNEL) \
	((Y) * (SIZE_X) * (NUM_CHANNELS) + (X) * (NUM_CHANNELS) + (CHANNEL))

void HSVToRGB(double H, double S, double V, double* R, double* G, double* B);

void calcMandelbrot(uint8_t* image, int sizeX, int sizeY, int start_row, int end_row) {
	const float left = -2.5, right = 1;
	const float bottom = -1, top = 1;

	for(int pixelY = start_row; pixelY < end_row; pixelY++) {
		// scale y pixel into mandelbrot coordinate system
		const float cy = (pixelY / (float)sizeY) * (top - bottom) + bottom;
		for(int pixelX = 0; pixelX < sizeX; pixelX++) {
			// scale x pixel into mandelbrot coordinate system
			const float cx = (pixelX / (float)sizeX) * (right - left) + left;
			float x = 0;
			float y = 0;
			int numIterations = 0;

			// Check if the distance from the origin becomes
			// greater than 2 within the max number of iterations.
			while((x * x + y * y <= 2 * 2) && (numIterations < MAX_ITER)) {
				float x_tmp = x * x - y * y + cx;
				y = 2 * x * y + cy;
				x = x_tmp;
				numIterations += 1;
			}

			// Normalize iteration and write it to pixel position
			double value = fabs((numIterations / (float)MAX_ITER)) * 200;

			double red = 0;
			double green = 0;
			double blue = 0;

			HSVToRGB(value, 1.0, 1.0, &red, &green, &blue);

			int channel = 0;
			image[IND(pixelY, pixelX, sizeY, sizeX, channel++)] = (uint8_t)(red * UINT8_MAX);
			image[IND(pixelY, pixelX, sizeY, sizeX, channel++)] = (uint8_t)(green * UINT8_MAX);
			image[IND(pixelY, pixelX, sizeY, sizeX, channel++)] = (uint8_t)(blue * UINT8_MAX);
		}
	}
}
int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int sizeX = DEFAULT_SIZE_X;
	int sizeY = DEFAULT_SIZE_Y;

	if(argc == 3) {
		sizeX = atoi(argv[1]);
		sizeY = atoi(argv[2]);
	} else if(rank == 0) {
		printf("No arguments given, using default size\n");
	}

	// Calculate the number of rows each process will handle
	int rows_per_process = (sizeY + size - 1) / size; // Ensure all rows are covered

	// Allocate memory for the local image based on the maximum possible size
	uint8_t* image = malloc(NUM_CHANNELS * sizeX * sizeY * sizeof(uint8_t));

	double start, end;
	start = MPI_Wtime();

	for(int i = 0; i < rows_per_process; i++) {
		int row = rank + i * size;
		if(row < sizeY) {
			calcMandelbrot(image, sizeX, sizeY, row, row + 1);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	uint8_t* full_image = NULL;
	if(rank == 0) {
		end = MPI_Wtime();
		double timeElapsed = end - start;
		printf("Time in master rank: %f\n", timeElapsed);
		full_image = malloc(NUM_CHANNELS * sizeX * sizeY * sizeof(uint8_t));
	}

	for(int row = 0; row < sizeY; row++) {
		int sourceRank = row % size;
		if(rank == 0) {
			if(sourceRank == 0) {
				// Copy row from local buffer to full image
				memcpy(full_image + row * sizeX * NUM_CHANNELS, image + row * sizeX * NUM_CHANNELS,
				       sizeX * NUM_CHANNELS);
			} else {
				// Receive row from other process
				MPI_Recv(full_image + row * sizeX * NUM_CHANNELS, NUM_CHANNELS * sizeX, MPI_UINT8_T,
				         sourceRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
		} else if(sourceRank == rank) {
			// Send row to root process
			MPI_Send(image + row * sizeX * NUM_CHANNELS, NUM_CHANNELS * sizeX, MPI_UINT8_T, 0, 0,
			         MPI_COMM_WORLD);
		}
	}

	if(rank == 0) {
		const int stride_bytes = 0;
		stbi_write_png("mandelbrot_mpi_static.png", sizeX, sizeY, NUM_CHANNELS, full_image,
		               stride_bytes);
		free(full_image);
	}

	free(image);
	MPI_Finalize();
}

void HSVToRGB(double H, double S, double V, double* R, double* G, double* B) {
	if(H >= 1.00) {
		V = 0.0;
		H = 0.0;
	}

	double step = 1.0 / 6.0;
	double vh = H / step;

	int i = (int)floor(vh);

	double f = vh - i;
	double p = V * (1.0 - S);
	double q = V * (1.0 - (S * f));
	double t = V * (1.0 - (S * (1.0 - f)));

	switch(i) {
		case 0: {
			*R = V;
			*G = t;
			*B = p;
			break;
		}
		case 1: {
			*R = q;
			*G = V;
			*B = p;
			break;
		}
		case 2: {
			*R = p;
			*G = V;
			*B = t;
			break;
		}
		case 3: {
			*R = p;
			*G = q;
			*B = V;
			break;
		}
		case 4: {
			*R = t;
			*G = p;
			*B = V;
			break;
		}
		case 5: {
			*R = V;
			*G = p;
			*B = q;
			break;
		}
	}
}
