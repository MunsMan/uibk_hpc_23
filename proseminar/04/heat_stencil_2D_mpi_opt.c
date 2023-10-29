#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef double value_t;

#define RESOLUTION 120

#define INDEX(i, j) ((i) + ((j) * bufferLengthX))

// -- vector utilities --

typedef value_t* Vector;

Vector createVector(int N);

void releaseVector(Vector X);
void factor(int n, int* fac1, int* fac2);

void printTemperature(Vector X, int n, int m);

void updateInnerCells(Vector A, Vector B, int rankLengthX, int rankLengthY, int bufferLengthX);

void updateOuterCells(Vector A, Vector B, int rankLengthX, int rankLengthY, int bufferLengthX,
                      int ranksX, int ranksY, int x, int y);
// -- simulation code ---

int main(int argc, char** argv) {
	// 'parsing' optional input parameter = problem size
	int N = 5000;
	if(argc > 1) {
		N = atoi(argv[1]);
	}

	int myRank, numProcs;

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

	if(myRank == 0) {
		printf("Setup: number of Processes = %d\n", numProcs);
	}

	int T = N * 100;
	printf("Computing heat-distribution for room size N=%d for T=%d timesteps\n", N, T);

	// ---------- setup ----------
	// assumed square problem space

	int dims[2] = { 0, 0 };
	MPI_Dims_create(numProcs, 2, dims);
	int ranksX = dims[0];
	int ranksY = dims[1];

	int periods[2] = { 0, 0 };
	MPI_Comm cartcomm;
	MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cartcomm);

	int coordinates[2];
	MPI_Cart_coords(cartcomm, myRank, 2, coordinates);
	int x = coordinates[0];
	int y = coordinates[1];

	int top, bottom, right, left;
	MPI_Cart_shift(cartcomm, 0, 1, &left, &right);
	MPI_Cart_shift(cartcomm, 1, 1, &top, &bottom);

	int rankLengthX = N / ranksX;
	int rankLengthY = N / ranksY;

	int bufferLengthX = rankLengthX + 2;
	int bufferLengthY = rankLengthY + 2;

	MPI_Datatype topBottom, leftRight;
	MPI_Type_contiguous(rankLengthX, MPI_DOUBLE, &topBottom);
	MPI_Type_commit(&topBottom);

	MPI_Type_vector(rankLengthY, 1, bufferLengthX, MPI_DOUBLE, &leftRight);
	MPI_Type_commit(&leftRight);

	MPI_Request req;

	// create a buffer for storing temperature fields
	Vector A = createVector(bufferLengthX * bufferLengthY);
	// create a second buffer for the computation
	Vector B = createVector(bufferLengthX * bufferLengthY);

	// set up initial conditions in A
	for(int j = 0; j < bufferLengthY; j++) {
		for(int i = 0; i < bufferLengthX; i++) {
			A[INDEX(i, j)] = 273; // temperature is 0° C everywhere (273 K)
			B[INDEX(i, j)] = 273; // temperature is 0° C everywhere (273 K)
		}
	}

	// and there is a heat source in one corner
	int source_x = N / 4;
	int source_y = N / 4;
	if(source_x / rankLengthX == x && source_y / rankLengthY == y) {
		source_x = source_x - x * rankLengthX + 1;
		source_y = source_y - y * rankLengthY + 1;
		A[INDEX(source_x, source_y)] = 273 + 60;
	} else {
		source_x = -1;
		source_y = -1;
	}

	int transCounts[4] = { 1, 1, 1, 1 };
	MPI_Datatype transTypes[4] = {
		leftRight,
		leftRight,
		topBottom,
		topBottom,
	};
	MPI_Datatype recTypes[4] = { leftRight, leftRight, topBottom, topBottom };
	MPI_Aint sendDisps[4] = { INDEX(1, 1), INDEX(rankLengthX, 1), INDEX(1, 1),
		                      INDEX(1, rankLengthY) };
	MPI_Aint recDisps[4] = { INDEX(0, 1), INDEX(rankLengthX + 1, 1), INDEX(1, 0),
		                     INDEX(1, rankLengthY + 1) };
	for(int i = 0; i < 4; i++) {
		sendDisps[i] *= sizeof(value_t);
		recDisps[i] *= sizeof(value_t);
	}
	// ---------- compute ----------

	double starttime, endtime;
	starttime = MPI_Wtime();

	// for each time step ..
	for(int t = 0; t < T; t++) {

		MPI_Ineighbor_alltoallw(A, transCounts, sendDisps, transTypes, A, transCounts, recDisps,
		                        transTypes, cartcomm, &req);

		updateInnerCells(A, B, rankLengthX, rankLengthY, bufferLengthX);

		MPI_Wait(&req, MPI_STATUS_IGNORE);

		updateOuterCells(A, B, rankLengthX, rankLengthY, bufferLengthX, ranksX, ranksY, x, y);

		if(source_y != -1) {
			B[INDEX(source_x, source_y)] = 333.0;
		}
		// swap matrices (just pointers, not content)
		Vector H = A;
		A = B;
		B = H;
		// show intermediate step
	}

	endtime = MPI_Wtime();
	Vector res = createVector(N * N);
	MPI_Datatype blockType, resizedType, subarrayType;

	int subArrayStarting[2] = { 1, 1 };
	int subArraySizes[2] = { bufferLengthX, bufferLengthY };
	int subArraySubSizes[2] = { rankLengthX, rankLengthY };
	MPI_Type_create_subarray(2, subArraySizes, subArraySubSizes, subArrayStarting, 1, MPI_DOUBLE,
	                         &subarrayType);
	MPI_Type_commit(&subarrayType);

	int starts[2] = { 0, 0 };
	int sizes[2] = { N, N };
	int subsizes[2] = { rankLengthX, rankLengthY };
	MPI_Type_create_subarray(2, sizes, subsizes, starts, 1, MPI_DOUBLE, &blockType);
	MPI_Type_commit(&blockType);
	MPI_Type_create_resized(blockType, 0, 1 * sizeof(value_t), &resizedType);
	MPI_Type_commit(&resizedType);

	int displacements[numProcs];
	int counts[numProcs];

	for(int rank = 0; rank < numProcs; rank++) {
		counts[rank] = 1;

		int coordinates[2];
		MPI_Cart_coords(cartcomm, rank, 2, coordinates);

		displacements[rank] = (coordinates[0] * rankLengthX + coordinates[1] * N * rankLengthY);
	}

	MPI_Gatherv(A, 1, subarrayType, res, counts, displacements, resizedType, 0, MPI_COMM_WORLD);

	int success = 1;
	if(myRank == 0) {

		printf("Wall: %f seconds\n", endtime - starttime);
		// ---------- check ----------

		printf("Final:\t\n");
		printTemperature(res, N, N);
		printf("\n");

		int success = 1;
		for(long long i = 0; i < N; i++) {
			for(long long j = 0; j < N; j++) {
				value_t temp = res[INDEX(i, j)];
				if(273 <= temp && temp <= 273 + 60) continue;
				success = 0;
				break;
			}
		}

		printf("Verification: %s\n", (success) ? "OK" : "FAILED");
	}

	// ---------- cleanup ----------
	MPI_Type_free(&topBottom);
	MPI_Type_free(&leftRight);
	MPI_Type_free(&subarrayType);
	MPI_Type_free(&blockType);
	MPI_Type_free(&resizedType);
	MPI_Finalize();
	releaseVector(A);
	releaseVector(B);
	releaseVector(res);

	// done
	return (success) ? EXIT_SUCCESS : EXIT_FAILURE;
}

Vector createVector(int N) {
	// create data and index vector
	return malloc(sizeof(value_t) * N);
}

void releaseVector(Vector m) {
	free(m);
}

void updateInnerCells(Vector A, Vector B, int rankLengthX, int rankLengthY, int bufferLengthX) {

	for(long long j = 2; j < rankLengthY; j++) {
		// .. we propagate the temperature
		Vector old_row = &A[INDEX(0, j)];
		Vector old_below = &A[INDEX(0, j + 1)];
		Vector old_above = &A[INDEX(0, j - 1)];
		Vector new_row = &B[INDEX(0, j)];
		for(long long i = 2; i < rankLengthX; i++) {
			// center stays constant (the heat is still on)

			value_t tl = old_row[i - 1];
			value_t tr = old_row[i + 1];
			value_t ta = old_above[i];
			value_t tb = old_below[i];
			new_row[i] = (tl + tr + tb + ta) / 4;
		}
	}
}

void updateOuterCells(Vector A, Vector B, int rankLengthX, int rankLengthY, int bufferLengthX,
                      int ranksX, int ranksY, int x, int y) {

	for(long long j = 1; j < rankLengthY + 1; j += rankLengthY - 1) {
		Vector old_row = &A[INDEX(0, j)];
		Vector old_below = y == ranksY && j == rankLengthY ? old_row : &A[INDEX(0, j + 1)];
		Vector old_above = y == 0 && j == 1 ? old_row : &A[INDEX(0, j - 1)];
		Vector new_row = &B[INDEX(0, j)];
		for(long long i = 2; i < rankLengthX; i++) {
			value_t tl = old_row[i - 1];
			value_t tr = old_row[i + 1];
			value_t ta = old_above[i];
			value_t tb = old_below[i];
			new_row[i] = (tl + tr + tb + ta) / 4;
		}
	}

	for(long long j = 1; j < rankLengthY + 1; j++) {
		for(long long i = 1; i < rankLengthX + 1; i += rankLengthX - 1) {
			value_t tc = A[INDEX(i, j)];
			value_t tl = x == 0 && i == 1 ? tc : A[INDEX(i - 1, j)];
			value_t tr = x == ranksX - 1 && i == rankLengthX ? tc : A[INDEX(i + 1, j)];
			value_t ta = y == 0 && j == 1 ? tc : A[INDEX(i, j - 1)];
			value_t tb = y == ranksY - 1 && j == rankLengthY ? tc : A[INDEX(i, j + 1)];

			// compute new temperature at current position
			B[INDEX(i, j)] = (tl + tr + tb + ta) / 4;
		}
	}
}
void printTemperature(Vector X, int bufferLengthX, int bufferLengthY) {
	const char* colors = " .-:=+*^X#%@";
	const int numColors = 12;

	// boundaries for temperature (for simplicity hard-coded)
	const value_t max = 273 + 30;
	const value_t min = 273 + 0;
	int W = RESOLUTION;
	int sH = bufferLengthY / W;
	int sW = bufferLengthX / W;
	for(int j = 0; j < W; j++) {
		printf("X");
		// actual room
		for(int i = 0; i < W; i++) {
			// get max temperature in this tile
			value_t max_t = 0;
			for(int y = sH * j; y < sH * j + sH; y++) {
				for(int x = sW * i; x < sW * i + sW; x++) {
					max_t = (max_t < X[INDEX(x, y)]) ? X[INDEX(x, y)] : max_t;
				}
			}
			value_t temp = max_t;

			// pick the 'color'
			int c = ((temp - min) / (max - min)) * numColors;
			c = (c >= numColors) ? numColors - 1 : ((c < 0) ? 0 : c);

			// print the average temperature
			printf("%c", colors[c]);
		}
		// right wall
		printf("X\n");
	}
}
