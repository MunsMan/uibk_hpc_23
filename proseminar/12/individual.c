#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if(argc != 3) {
		if(rank == 0) {
			fprintf(stderr, "Usage: %s <buffer_size> <base_dir>\n", argv[0]);
		}
		MPI_Finalize();
		return 1;
	}

	int buffer_size = atoi(argv[1]);
	char* base_dir = argv[2];
	char* buffer = (char*)malloc(buffer_size * sizeof(char));

	for(int i = 0; i < buffer_size; ++i) {
		buffer[i] = (char)(rank + 'A');
	}

	char filename[256];
	sprintf(filename, "%s/rank_%d_file.txt", base_dir, rank);
	FILE* file = fopen(filename, "w");
	if(file == NULL) {
		perror("Error opening file");
		free(buffer);
		MPI_Finalize();
		return 1;
	}

	double start_time = MPI_Wtime();

	fwrite(buffer, sizeof(char), buffer_size, file);
	fflush(file);

	for(int i = 0; i < 9; i++) {
		fseek(file, 0, SEEK_SET);
		fread(buffer, sizeof(char), buffer_size, file);
		fseek(file, 0, SEEK_SET);
		fwrite(buffer, sizeof(char), buffer_size, file);
		fflush(file);
	}

	double end_time = MPI_Wtime();

	fclose(file);
	free(buffer);

	double fwrite_time = end_time - start_time;

	double max_fwrite_time;
	MPI_Allreduce(&fwrite_time, &max_fwrite_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

	if(rank == 0) {
		printf("Maximum time for %d bytes: %f seconds\n", buffer_size, max_fwrite_time);
	}

	MPI_Finalize();
	return 0;
}
