#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "octree.h"

#define epsilon \
	1e-6 // This is the softening length,to prevent the force from blowing up as the distance goes
	     // to zero.

#define NUM_PARTICLES 100
#define NUM_STEPS 100
#define G 1

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

void move_particle(Octree* particle, List* neigbours) {
	Vector3D force = { 0, 0, 0 };
	ListNode* node = neigbours->first;
	CenterGravity* neigbour;
	while(node) {
		neigbour = node->element;
		if(neigbour == &particle->center_gravity) {
			node = node->next;
			continue;
		}
		Vector3D distance_vector = vec_sub(neigbour->position, particle->center_gravity.position);

		double distanceSquared = distance_vector.x * distance_vector.x +
		                         distance_vector.y * distance_vector.y +
		                         distance_vector.z * distance_vector.z + epsilon;
		double F = G * particle->center_gravity.mass * neigbour->mass / distanceSquared;

		double distanceMagnitude = sqrt(distanceSquared - epsilon);

		if(distanceMagnitude < 1e-10) {
			distanceMagnitude = 1e-10;
		}
		force.x += F * (distance_vector.x / distanceMagnitude);
		force.y += F * (distance_vector.y / distanceMagnitude);
		force.z += F * (distance_vector.z / distanceMagnitude);
		node = node->next;
	}
	Particle* p = particle->particle;
	p->velocity.x += force.x / p->mass;
	p->velocity.y += force.y / p->mass;
	p->velocity.z += force.z / p->mass;

	p->position.x += p->velocity.x;
	p->position.y += p->velocity.y;
	p->position.z += p->velocity.z;
}

int main(void) {
	Particle particles[NUM_PARTICLES];
	FILE* file = fopen("data.dat", "w");
	if(!file) {
		printf("Error opening file\n");
		return 1;
	}

	srand(time(NULL));
	initialize_particles(particles);
	// init_test_setup(particles);

	clock_t start = clock();

	Vector3D max_position = { .x = 100, .y = 100, .z = 100 };
	Vector3D min_position = { .x = 0, .y = 0, .z = 0 };

	for(int step = 0; step < NUM_STEPS; step++) {

		Octree* octree = init_tree(max_position, min_position);
		for(int i = 0; i < NUM_PARTICLES; i++) {
			insert(octree, &(particles[i]));
		}
		IteratorState* state = iterator_leaf_init(octree);
		while(iterator_leaf_has_next(state)) {
			Octree* node = iterator_leaf_next(state);
			List* neigbours = effecting_neigbours(node, 1);
			move_particle(node, neigbours);
			list_free(neigbours);
		}
		free_tree(octree);
		free(state);
		printf("%f %f %f\n", particles[0].position.x, particles[0].position.y,
		       particles[0].position.z);
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
