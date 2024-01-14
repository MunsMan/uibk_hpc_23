
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <buffer_size>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int buffer_size = atoi(argv[1]);
    char *buffer = (char *)malloc(buffer_size * sizeof(char));

    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] = (char)(rank + 'A');
    }

    MPI_File mpi_file;
    MPI_File_open(MPI_COMM_WORLD, "shared_file.txt", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &mpi_file);

    MPI_Offset offset = rank * buffer_size * sizeof(char);

    double start_time = MPI_Wtime();

    MPI_File_write_at(mpi_file, offset, buffer, buffer_size, MPI_CHAR, MPI_STATUS_IGNORE);

    double end_time = MPI_Wtime();

    MPI_File_close(&mpi_file);
    free(buffer);

    double fwrite_time = end_time - start_time;

    double max_fwrite_time;
    MPI_Allreduce(&fwrite_time, &max_fwrite_time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    if (rank == 0) {
		printf("Maximum time for %d bytes: %f seconds\n", buffer_size, max_fwrite_time);
    }

    MPI_Finalize();
    return 0;
}
