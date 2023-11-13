#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define G 1

typedef struct {
	double x, y, z;
} Vector3D;

int main(int argc, char const* argv[]) {
	if(argc < 2) {
		printf("Please run with: ./nbody <numParticles> <iterations>\n");
		exit(1);
	}

	FILE* file = fopen("data.dat", "w");
	if(!file) {
		printf("Error opening file\n");
		return 1;
	}

	srand(time(NULL));
	int numParticles = atoi(argv[1]);
	int iterations = atoi(argv[2]);

	Vector3D* forces = malloc(numParticles * sizeof(Vector3D));
	Vector3D* positions = malloc(numParticles * sizeof(Vector3D));
	Vector3D* velocities = malloc(numParticles * sizeof(Vector3D));
	double* masses = malloc(numParticles * sizeof(double));

	double* preFactors = malloc(numParticles * sizeof(double));
	double* massInverses = malloc(numParticles * sizeof(double));

	for(int i = 0; i < numParticles; i++) {
		positions[i].x = (double)rand() / RAND_MAX * 100;
		positions[i].y = (double)rand() / RAND_MAX * 100;
		positions[i].z = (double)rand() / RAND_MAX * 100;

		velocities[i].x = 0;
		velocities[i].y = 0;
		velocities[i].z = 0;

		masses[i] = (double)rand() / RAND_MAX + 1;
    preFactors[i] = -G * masses[i];
    massInverses[i] = 1 / masses[i];
	}

	clock_t start = clock();

	for(int i = 0; i < iterations; i++) {

		for(int n = 0; n < numParticles; n++) {
			forces[n] = (Vector3D){ 0, 0, 0 };
		}

		for(int n = 0; n < numParticles; n++) {
			for(int m = 0; m < numParticles; m++) {
				if(m <= n) {
					continue;
				}

				double xDiff = positions[n].x - positions[m].x;
				double yDiff = positions[n].y - positions[m].y;
				double zDiff = positions[n].z - positions[m].z;

				double dist = sqrt(xDiff * xDiff + yDiff * yDiff + zDiff * zDiff);
				double distCubed = dist * dist * dist;

				double forceScaled = preFactors[n] * masses[m] / distCubed;

				double forceX = forceScaled * xDiff;
				double forceY = forceScaled * yDiff;
				double forceZ = forceScaled * zDiff;

				forces[n].x += forceX;
				forces[n].y += forceY;
				forces[n].z += forceZ;
				forces[m].x -= forceX;
				forces[m].y -= forceY;
				forces[m].z -= forceZ;
			}
		}

		for(int n = 0; n < numParticles; n++) {
			velocities[n].x += forces[n].x * massInverses[n];
			velocities[n].y += forces[n].y * massInverses[n];
			velocities[n].z += forces[n].z * massInverses[n];
			positions[n].x += velocities[n].x;
			positions[n].y += velocities[n].y;
			positions[n].z += velocities[n].z;

			fprintf(file, "%f %f %f\n", positions[n].x, positions[n].y, positions[n].z);
		}
		fprintf(file, "\n\n");
	}

	clock_t end = clock();
	printf("Simulation time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);

	free(forces);
	free(positions);
	free(velocities);
	free(masses);

	return EXIT_SUCCESS;
}
