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

#define ROW_CHUNK_SIZE 1

#define IND(Y, X, SIZE_Y, SIZE_X, CHANNEL) \
	((Y) * (SIZE_X) * (NUM_CHANNELS) + (X) * (NUM_CHANNELS) + (CHANNEL))

struct work {
	int start_row;
	int end_row;
};

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

	int rank, numRanks;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numRanks);

	int sizeX = DEFAULT_SIZE_X;
	int sizeY = DEFAULT_SIZE_Y;

	if(argc == 3) {
		sizeX = atoi(argv[1]);
		sizeY = atoi(argv[2]);
	} else {
		printf("No arguments given, using default size\n");
	}

	MPI_Datatype work_type;
	MPI_Type_contiguous(2, MPI_INT, &work_type);
	MPI_Type_commit(&work_type);

	uint8_t* full_image = malloc(NUM_CHANNELS * sizeX * sizeY * sizeof(uint8_t));
	int* rowSources = malloc(sizeY * sizeof(int));
	int local_rows = 0;

	if(rank == 0) {
		double start, end;
		start = MPI_Wtime();

		int distributed_rows = 0;

		for(int i = 1; i < numRanks; i++) {
			struct work w;
			w.start_row = distributed_rows;
			w.end_row = w.start_row + ROW_CHUNK_SIZE;
			MPI_Send(&w, 1, work_type, i, 0, MPI_COMM_WORLD);
			distributed_rows += ROW_CHUNK_SIZE;
		}
		int finished_workers = 0;
		do {
			MPI_Status status;
			MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			int source = status.MPI_SOURCE;
			int start_row;
			MPI_Recv(&start_row, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			full_image[start_row * sizeX * NUM_CHANNELS] = source;
			if(distributed_rows <= sizeY) {
				struct work w;
				w.start_row = distributed_rows;
				w.end_row = w.start_row + ROW_CHUNK_SIZE;
				MPI_Send(&w, 1, work_type, source, 0, MPI_COMM_WORLD);
				distributed_rows += ROW_CHUNK_SIZE;
			} else {
				struct work w;
				w.start_row = -1;
				MPI_Send(&w, 1, work_type, source, 0, MPI_COMM_WORLD);
				finished_workers++;
			}
		} while(finished_workers < numRanks - 1);

		end = MPI_Wtime();
		double timeElapsed = end - start;
		printf("Time in master rank: %f\n", timeElapsed);

	} else {
		struct work w;
		MPI_Recv(&w, 1, work_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		while(w.start_row != -1) {
			calcMandelbrot(full_image, sizeX, sizeY, w.start_row, w.end_row);
			rowSources[local_rows++] = w.start_row;
			MPI_Send(&w.start_row, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
			MPI_Recv(&w, 1, work_type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	}

	uint8_t* local_image = malloc(NUM_CHANNELS * sizeX * local_rows * sizeof(uint8_t));
	for(int i = 0; i < local_rows; i++) {
		for(int j = 0; j < sizeX * NUM_CHANNELS; j++) {
			local_image[IND(i, j, local_rows, sizeX, 0)] = full_image[IND(i, j, sizeY, sizeX, 0)];
		}
	}

	uint8_t* all_images = NULL;
	if(rank != 0) all_images = malloc(numRanks * NUM_CHANNELS * sizeX * sizeY * sizeof(uint8_t));

	MPI_Gather(full_image, NUM_CHANNELS * sizeX * sizeY, MPI_UINT8_T, all_images,
	           NUM_CHANNELS * sizeX * sizeY, MPI_UINT8_T, 0, MPI_COMM_WORLD);

	if(rank == 0) {
		for(int y = 0; y < sizeY; y++) {
			int source_rank = full_image[y * sizeX * NUM_CHANNELS];
			for(int i = 0; i < sizeX; i++) {
				full_image[IND(y, i, sizeY, sizeX, 0)] =
				    all_images[IND(y + source_rank * sizeY, i, sizeY, sizeX, 0)];
				full_image[IND(y, i, sizeY, sizeX, 1)] =
				    all_images[IND(y + source_rank * sizeY, i, sizeY, sizeX, 1)];
				full_image[IND(y, i, sizeY, sizeX, 2)] =
				    all_images[IND(y + source_rank * sizeY, i, sizeY, sizeX, 2)];
			}
		}
		const int stride_bytes = 0;
		stbi_write_png("mandelbrot_mpi_balanced.png", sizeX, sizeY, NUM_CHANNELS, full_image,
		               stride_bytes);
		free(all_images);
	}

	MPI_Type_free(&work_type);
	free(full_image);
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
