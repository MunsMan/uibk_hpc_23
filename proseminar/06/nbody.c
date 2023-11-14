#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define epsilon \
	1e-6 // This is the softening length,to prevent the force from blowing up as the distance goes
	     // to zero.

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

typedef struct {
	Vector3D max_position;
	Vector3D min_position;
	Particle* particle;
} Wolrd;

void init_test_setup(Particle particles[]) {
	particles[0].position.x = 50;
	particles[0].position.y = 50;
	particles[0].position.z = 50;
	particles[0].velocity.x = 0;
	particles[0].velocity.y = 0;
	particles[0].velocity.z = 0;
	particles[0].mass = 1000;

	particles[1].position.x = 10;
	particles[1].position.y = 10;
	particles[1].position.z = 50;
	particles[1].velocity.x = 1;
	particles[1].velocity.y = 1;
	particles[1].velocity.z = 1;
	particles[1].mass = 1;
}

void initialize_particles(Particle particles[]) {
	for(int i = 0; i < NUM_PARTICLES; i++) {
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

				double distanceSquared = distance.x * distance.x + distance.y * distance.y +
				                         distance.z * distance.z + epsilon;
				double F = G * particles[i].mass * particles[j].mass / distanceSquared;

				double distanceMagnitude = sqrt(distanceSquared - epsilon);

				if(distanceMagnitude < 1e-10) {
					distanceMagnitude = 1e-10;
				}
				force.x += F * (distance.x / distanceMagnitude);
				force.y += F * (distance.y / distanceMagnitude);
				force.z += F * (distance.z / distanceMagnitude);
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

int main(void) {
	Particle particles[NUM_PARTICLES];
	Wolrd world = { .max_position = { 100.0, 100.0, 100.0 },
		            .min_position = { 0.0, 0.0, 0.0 },
		            .particle = particles };
	FILE* file = fopen("data.dat", "w");
	if(!file) {
		printf("Error opening file\n");
		return 1;
	}

	srand(time(NULL));
	initialize_particles(particles);
	// init_test_setup(particles);

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
