#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void individual_file(int rank, char* buffer, uint64_t buf_size, char* base_dir);
void individual_file_pointer(int rank, char* buffer, uint64_t buf_size, char* base_dir);
void shared_file_pointer_non_collective(int rank, char* buffer, uint64_t buf_size, char* base_dir);
void shared_file_pointer_collective(int rank, char* buffer, uint64_t buf_size, char* base_dir);

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	// Define buffer size and allocate memory
	if(argc != 3) {
		if(rank == 0) {
			fprintf(stderr, "Usage: %s <buffer_size> <base_dir>\n", argv[0]);
		}
		MPI_Finalize();
		return 1;
	}

	uint64_t buf_size = strtoul(argv[1], NULL, 10);
	char* base_dir = argv[2];

	char* buffer = (char*)malloc(buf_size * sizeof(char));

	// Initialize buffer based on rank
	for(uint64_t i = 0; i < buf_size; ++i) {
		buffer[i] = 'A' + rank;
	}

	// Individual file
	double start = MPI_Wtime();
	individual_file(rank, buffer, buf_size, base_dir);
	printf("Individual file: %fs\n", MPI_Wtime() - start);
	char filename[100];
	sprintf(filename, "%s/individual_file_%d", base_dir, rank);
	remove(filename);
	MPI_Barrier(MPI_COMM_WORLD);

	// Individual file pointer
	start = MPI_Wtime();
	individual_file_pointer(rank, buffer, buf_size, base_dir);
	MPI_Barrier(MPI_COMM_WORLD);
	if(rank == 0) {
		printf("Individual file pointer: %fs\n", MPI_Wtime() - start);
		char filename[100];
		sprintf(filename, "%s/individual_file_pointer", base_dir);
		remove(filename);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	// Shared file pointer non-collective
	start = MPI_Wtime();
	shared_file_pointer_non_collective(rank, buffer, buf_size, base_dir);
	MPI_Barrier(MPI_COMM_WORLD);
	if(rank == 0) {
		printf("Shared file pointer non-collective: %fs\n", MPI_Wtime() - start);
		char filename[100];
		sprintf(filename, "%s/individual_file_pointer_non_collective", base_dir);
		remove(filename);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	// Shared file pointer collective
	start = MPI_Wtime();
	shared_file_pointer_collective(rank, buffer, buf_size, base_dir);
	MPI_Barrier(MPI_COMM_WORLD);
	if(rank == 0) {
		printf("Shared file pointer collective: %fs\n", MPI_Wtime() - start);
		char filename[100];
		sprintf(filename, "%s/individual_file_pointer_collective", base_dir);
		remove(filename);
	}

	// Clean up
	free(buffer);
	MPI_Finalize();

	return EXIT_SUCCESS;
}

void individual_file(int rank, char* buffer, uint64_t buf_size, char* base_dir) {
	char filename[100];
	sprintf(filename, "%s/individual_file_%d", base_dir, rank);
	// Initialize file
	FILE* file = fopen(filename, "wr+");
	off_t offset = 0;
	if(file == NULL) {
		printf("Error opening file!\n%s\n", filename);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	fseeko(file, offset, SEEK_SET);
	fwrite(buffer, sizeof(char), buf_size, file);
	fflush(file);

	for(int i = 0; i < 9; ++i) {
		// Read Operation
		fseeko(file, offset, SEEK_SET);
		fread(buffer, sizeof(char), buf_size, file);
		// Write Operation
		fseeko(file, offset, SEEK_SET);
		fwrite(buffer, sizeof(char), buf_size, file);
		fflush(file);
	}
	fclose(file);
}

void individual_file_pointer(int rank, char* buffer, uint64_t buf_size, char* base_dir) {
	char filename[100];
	sprintf(filename, "%s/individual_file_pointer", base_dir);
	MPI_File file;
	MPI_Status status;
	MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &file);
	if(file == NULL) {
		printf("Error opening file!\n%s\n", filename);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	MPI_Offset offset = rank * buf_size;
	MPI_File_seek(file, offset, MPI_SEEK_SET);
	MPI_File_write(file, buffer, buf_size, MPI_CHAR, &status);
	MPI_File_sync(file);

	for(int i = 0; i < 9; ++i) {
		// Read Operation
		MPI_File_seek(file, offset, MPI_SEEK_SET);
		MPI_File_read(file, buffer, buf_size, MPI_CHAR, &status);
		// Write Operation
		MPI_File_seek(file, offset, MPI_SEEK_SET);
		MPI_File_write(file, buffer, buf_size, MPI_CHAR, &status);
		MPI_File_sync(file);
	}
	MPI_File_close(&file);
}

void shared_file_pointer_non_collective(int rank, char* buffer, uint64_t buf_size, char* base_dir) {
	char filename[100];
	sprintf(filename, "%s/individual_file_pointer_non_collective", base_dir);
	MPI_File file;
	MPI_Status status;
	MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &file);
	if(file == NULL) {
		printf("Error opening file!\n%s\n", filename);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	MPI_Offset offset = rank * buf_size;
	MPI_File_seek_shared(file, offset, MPI_SEEK_SET);
	MPI_File_write_shared(file, buffer, buf_size, MPI_CHAR, &status);
	MPI_File_sync(file);

	for(int i = 0; i < 9; ++i) {
		// Read Operation
		MPI_File_seek_shared(file, offset, MPI_SEEK_SET);
		MPI_File_read_shared(file, buffer, buf_size, MPI_CHAR, &status);
		// Write Operation
		MPI_File_seek_shared(file, offset, MPI_SEEK_SET);
		MPI_File_write_shared(file, buffer, buf_size, MPI_CHAR, &status);
		MPI_File_sync(file);
	}
	MPI_File_close(&file);
}

void shared_file_pointer_collective(int rank, char* buffer, uint64_t buf_size, char* base_dir) {
	char filename[100];
	sprintf(filename, "%s/individual_file_pointer_collective", base_dir);
	MPI_File file;
	MPI_Status status;
	MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, MPI_INFO_NULL, &file);
	if(file == NULL) {
		printf("Error opening file!\n%s\n", filename);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
	MPI_Offset offset = rank * buf_size;
	MPI_File_seek_shared(file, offset, MPI_SEEK_SET);
	MPI_File_write_ordered(file, buffer, buf_size, MPI_CHAR, &status);
	MPI_File_sync(file);

	for(int i = 0; i < 9; ++i) {
		// Read Operation
		MPI_File_seek_shared(file, offset, MPI_SEEK_SET);
		MPI_File_read_ordered(file, buffer, buf_size, MPI_CHAR, &status);
		// Write Operation
		MPI_File_seek_shared(file, offset, MPI_SEEK_SET);
		MPI_File_write_ordered(file, buffer, buf_size, MPI_CHAR, &status);
		MPI_File_sync(file);
	}
	MPI_File_close(&file);
}
