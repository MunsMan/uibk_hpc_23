#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define G 1
#define IMBALANCE_FACTOR 0.5

typedef struct {
	double x, y, z;
} Vector3D;

typedef struct {
	double mass;
	Vector3D position;
	Vector3D velocity;
	Vector3D force;
} Particle;

int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	int numRanks, myRank;
	MPI_Comm_size(MPI_COMM_WORLD, &numRanks);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

	if(argc < 3) {
		printf("Please run with: ./nbody <numParticles> <iterations>\n");
		exit(1);
	}

	FILE* file;
	if(myRank == 0) {
		file = fopen("data.dat", "w");
		if(!file) {
			printf("Error opening file\n");
			MPI_Finalize();
			return 1;
		}
	}

	srand(time(NULL));
	int numParticles = atoi(argv[1]);
	int iterations = atoi(argv[2]);

	if(numParticles % numRanks != 0) {
		printf("Number of particles must be divisible by number of ranks\n");
		MPI_Finalize();
		return 1;
	}

	int localNumParticles = numParticles / numRanks;

	Particle* localParticles = malloc(localNumParticles * sizeof(Particle));
	Particle* allParticles = malloc(numParticles * sizeof(Particle));

	Vector3D* localPositions = malloc(localNumParticles * sizeof(Vector3D));
	Vector3D* allPositions = malloc(numParticles * sizeof(Vector3D));

	if(myRank == 0) {
		for(int i = 0; i < numParticles; i++) {

			allParticles[i].position.x = (double)rand() / RAND_MAX * 100;
			allParticles[i].position.y = (double)rand() / RAND_MAX * 100;
			allParticles[i].position.z = (double)rand() / RAND_MAX * 100;

			allParticles[i].velocity.x = 0;
			allParticles[i].velocity.y = 0;
			allParticles[i].velocity.z = 0;

			allParticles[i].mass = (double)rand() / RAND_MAX + 1;
		}
	}

	MPI_Bcast(allParticles, numParticles * sizeof(Particle), MPI_BYTE, 0, MPI_COMM_WORLD);

	for(int i = 0; i < numParticles; i++) {
		allPositions[i] = allParticles[i].position;
	}

	int startIndex = myRank * localNumParticles;
	int endIndex = startIndex + localNumParticles;

	for(int i = startIndex, j = 0; i < endIndex; i++, j++) {
		localParticles[j] = allParticles[i];
		localPositions[j] = allPositions[i];
	}

	double starttime, endtime;
	starttime = MPI_Wtime();

	for(int i = 0; i < iterations; i++) {

		for(int n = 0; n < localNumParticles; n++) {
			localParticles[n].force = (Vector3D){ 0, 0, 0 };
		}

		for(int n = 0; n < localNumParticles; n++) {
			for(int m = 0; m < numParticles; m++) {

				if(m == startIndex + n) continue;

				double xDiff = localPositions[n].x - allParticles[m].position.x;
				double yDiff = localPositions[n].y - allParticles[m].position.y;
				double zDiff = localPositions[n].z - allParticles[m].position.z;

				double dist = sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff) + 1e-5;
				double distCubed = dist * dist * dist;

				double forceScaled = -G * localParticles[n].mass * allParticles[m].mass / distCubed;

				double forceX = forceScaled * xDiff;
				double forceY = forceScaled * yDiff;
				double forceZ = forceScaled * zDiff;

				localParticles[n].force.x += forceX;
				localParticles[n].force.y += forceY;
				localParticles[n].force.z += forceZ;
			}
		}

		MPI_Allgather(localPositions, localNumParticles * sizeof(Vector3D), MPI_BYTE, allPositions,
		              localNumParticles * sizeof(Vector3D), MPI_BYTE, MPI_COMM_WORLD);

		for(int n = 0; n < localNumParticles; n++) {
			localParticles[n].velocity.x += localParticles[n].force.x / localParticles[n].mass;
			localParticles[n].velocity.y += localParticles[n].force.y / localParticles[n].mass;
			localParticles[n].velocity.z += localParticles[n].force.z / localParticles[n].mass;

			localPositions[n].x += localParticles[n].velocity.x;
			localPositions[n].y += localParticles[n].velocity.y;
			localPositions[n].z += localParticles[n].velocity.z;
		}

		if(myRank == 0) {
			for(int n = 0; n < numParticles; n++) {
				fprintf(file, "%f %f %f\n", allPositions[n].x, allPositions[n].y,
				        allPositions[n].z);
			}
			fprintf(file, "\n\n");
		}
	}
	if(myRank == 0) {
		endtime = MPI_Wtime();
		printf("Simulation time: %f seconds\n", endtime - starttime);
		free(allParticles);
		fclose(file);
	}
	free(localParticles);
	free(localPositions);
	free(allPositions);
	MPI_Finalize();

	return EXIT_SUCCESS;
}
