#include <mpi.h>
#include <stdint.h>
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

	uint64_t buffer_size_per_rank = strtoul(argv[1], NULL, 10);
	char* base_dir = argv[2];
	uint64_t total_size = buffer_size_per_rank * size;
	char* buffer = (char*)malloc(buffer_size_per_rank * sizeof(char));
	char* total_buffer = NULL;
	FILE* file = NULL;

	for(int i = 0; i < buffer_size_per_rank; ++i) {
		buffer[i] = (char)(rank + 'A');
	}

	if(rank == 0) {
		char filename[256];
		sprintf(filename, "%s/collective_file.txt", base_dir);
		total_buffer = (char*)malloc(total_size * sizeof(char));
		file = fopen(filename, "w+");
		if(file == NULL) {
			perror("Error opening file");
			free(buffer);
			free(total_buffer);
			MPI_Finalize();
			return 1;
		}
	}

	double start_time = MPI_Wtime(), end_time;

	for(int i = 0; i < 10; ++i) {
		MPI_Gather(buffer, buffer_size_per_rank, MPI_CHAR, total_buffer, buffer_size_per_rank,
		           MPI_CHAR, 0, MPI_COMM_WORLD);

		if(rank == 0) {
			fwrite(total_buffer, sizeof(char), total_size, file);
			fflush(file);
			if(i < 9) {
				fseek(file, 0, SEEK_SET);
				fread(total_buffer, sizeof(char), total_size, file);
				fseek(file, 0, SEEK_SET);
			}
		}

		MPI_Scatter(total_buffer, buffer_size_per_rank, MPI_CHAR, buffer, buffer_size_per_rank,
		            MPI_CHAR, 0, MPI_COMM_WORLD);
	}

	if(rank == 0) {
		end_time = MPI_Wtime();
		printf("Total time for %lld bytes: %f seconds\n", total_size, end_time - start_time);
		fclose(file);
		free(total_buffer);
	}

	free(buffer);
	MPI_Finalize();
	return 0;
}
