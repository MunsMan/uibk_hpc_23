#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_PARTICLES 5000
#define NUM_STEPS 100
#define G 1

typedef struct {
	double x, y, z;
} Vector3D;

typedef struct {
	Vector3D position;
	Vector3D velocity;
	double mass;
} Particle;

void initialize_particles(Particle particles[], int num_particles) {
	for(int i = 0; i < num_particles; i++) {
		particles[i].position.x = (double)rand() / RAND_MAX * 100;
		particles[i].position.y = (double)rand() / RAND_MAX * 100;
		particles[i].position.z = (double)rand() / RAND_MAX * 100;
		particles[i].velocity.x = 0;
		particles[i].velocity.y = 0;
		particles[i].velocity.z = 0;
		particles[i].mass = (double)rand() / RAND_MAX + 1;
	}
}

void move_particles(Particle particles[]) {
	for(int i = 0; i < NUM_PARTICLES; i++) {
		Vector3D force = { 0, 0, 0 };
		for(int j = 0; j < NUM_PARTICLES; j++) {
			if(i != j) {
				Vector3D distance;
				distance.x = particles[j].position.x - particles[i].position.x;
				distance.y = particles[j].position.y - particles[i].position.y;
				distance.z = particles[j].position.z - particles[i].position.z;

				double distanceSquared =
				    distance.x * distance.x + distance.y * distance.y + distance.z * distance.z;
				double F = G * particles[i].mass * particles[j].mass / distanceSquared;

				double distanceMagnitude = sqrt(distanceSquared);
				Vector3D unit_distance;
				unit_distance.x = distance.x / distanceMagnitude;
				unit_distance.y = distance.y / distanceMagnitude;
				unit_distance.z = distance.z / distanceMagnitude;

				force.x += F * unit_distance.x;
				force.y += F * unit_distance.y;
				force.z += F * unit_distance.z;
			}
		}
		particles[i].velocity.x += force.x / particles[i].mass;
		particles[i].velocity.y += force.y / particles[i].mass;
		particles[i].velocity.z += force.z / particles[i].mass;

		particles[i].position.x += particles[i].velocity.x;
		particles[i].position.y += particles[i].velocity.y;
		particles[i].position.z += particles[i].velocity.z;
	}
}

int main() {
	Particle particles[NUM_PARTICLES];
	FILE* file = fopen("data.dat", "w");
	if(!file) {
		printf("Error opening file\n");
		return 1;
	}

	srand(time(NULL));
	initialize_particles(particles, NUM_PARTICLES);

	clock_t start = clock();

	for(int step = 0; step < NUM_STEPS; step++) {
		move_particles(particles);
		for(int i = 0; i < NUM_PARTICLES; i++) {
			fprintf(file, "%f %f %f\n", particles[i].position.x, particles[i].position.y,
			        particles[i].position.z);
		}
		fprintf(file, "\n\n");
	}

	clock_t end = clock();
	printf("Simulation time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);

	fclose(file);
	return 0;
}
