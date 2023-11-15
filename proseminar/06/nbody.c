#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define G 1 // 6.67430e-11 // gravitational constant
#define THETA 0.6
#define NUM_PARTICLES 5000
#define NUM_STEPS 100

typedef struct {
	double x;
	double y;
	double z;
} Vector3D;

typedef struct {
	double mass;
	Vector3D position;
	Vector3D velocity;
} Particle;

typedef struct Node {
	Vector3D center;
	double size;
	double mass;
	Vector3D center_of_mass;
	struct Node* children[8];
} Node;

Node* create_node(Vector3D center, double size) {
	Node* node = (Node*)malloc(sizeof(Node));
	node->center.x = center.x;
	node->center.y = center.y;
	node->center.z = center.z;
	node->center_of_mass.x = 0.0;
	node->center_of_mass.y = 0.0;
	node->center_of_mass.z = 0.0;
	node->size = size;
	node->mass = 0.0;
	for(int i = 0; i < 8; ++i) {
		node->children[i] = NULL;
	}
	return node;
}

void free_tree(Node* node) {
	if(node == NULL) {
		return;
	}
	for(int i = 0; i < 8; ++i) {
		free_tree(node->children[i]);
	}
	free(node);
}

double distance(Vector3D position1, Vector3D position2) {
	double dx = position1.x - position2.x;
	double dy = position1.y - position2.y;
	double dz = position1.z - position2.z;
	return sqrt(dx * dx + dy * dy + dz * dz);
}

void update_force(Particle* particle, Node* node) {
	if(node->mass == 0.0) {
		return;
	}

	Vector3D direction;
	direction.x = node->center_of_mass.x - particle->position.x;
	direction.y = node->center_of_mass.y - particle->position.y;
	direction.z = node->center_of_mass.z - particle->position.z;

	double distance_particle_node = distance(particle->position, node->center);
	if(node->size / distance_particle_node < THETA) {
		double force_magnitude =
		    G * (particle->mass * node->mass) / (distance_particle_node * distance_particle_node);
		particle->velocity.x += force_magnitude * direction.x / distance_particle_node;
		particle->velocity.y += force_magnitude * direction.y / distance_particle_node;
		particle->velocity.z += force_magnitude * direction.z / distance_particle_node;
	} else {
		int num_children = 0;
		for(int i = 0; i < 8; ++i) {
			if(node->children[i] != NULL) {
				update_force(particle, node->children[i]);
				num_children++;
			}
		}
		if(!num_children) {
			double force_magnitude = G * (particle->mass * node->mass) /
			                         (distance_particle_node * distance_particle_node);
			particle->velocity.x += force_magnitude * direction.x / distance_particle_node;
			particle->velocity.y += force_magnitude * direction.y / distance_particle_node;
			particle->velocity.z += force_magnitude * direction.z / distance_particle_node;
		}
	}
}

void build_tree(Particle** particles, int num_particles, Node* node) {
	if(num_particles < 1) return;
	if(num_particles == 1) {
		node->mass = particles[0]->mass;
		node->center_of_mass.x = particles[0]->position.x;
		node->center_of_mass.y = particles[0]->position.y;
		node->center_of_mass.z = particles[0]->position.z;
		return;
	}

	for(int i = 0; i < 8; ++i) {
		Vector3D sub_center;
		sub_center.x = node->center.x + 0.5 * node->size * ((i & 1 << (2)) ? 1 : -1);
		sub_center.y = node->center.y + 0.5 * node->size * ((i & 1 << (1)) ? 1 : -1);
		sub_center.z = node->center.z + 0.5 * node->size * ((i & 1 << (0)) ? 1 : -1);

		node->children[i] = create_node(sub_center, 0.5 * node->size);

		int num_sub_particles = 0;
		Particle* sub_particles[num_particles];
		for(int j = 0; j < num_particles; ++j) {
			if(fabs(particles[j]->position.x - sub_center.x) < 0.5 * node->size &&
			   fabs(particles[j]->position.y - sub_center.y) < 0.5 * node->size &&
			   fabs(particles[j]->position.z - sub_center.z) < 0.5 * node->size) {
				sub_particles[num_sub_particles++] = particles[j];
			}
		}
		build_tree(sub_particles, num_sub_particles, node->children[i]);
	}

	for(int i = 0; i < 8; ++i) {
		if(node->children[i] != NULL) {
			node->mass += node->children[i]->mass;
			node->center_of_mass.x += node->children[i]->mass * node->children[i]->center_of_mass.x;
			node->center_of_mass.y += node->children[i]->mass * node->children[i]->center_of_mass.y;
			node->center_of_mass.z += node->children[i]->mass * node->children[i]->center_of_mass.z;
		}
	}

	if(node->mass != 0.0) {
		node->center_of_mass.x /= node->mass;
		node->center_of_mass.y /= node->mass;
		node->center_of_mass.z /= node->mass;
	}
}

void barnes_hut(Particle** particles, int num_particles) {
	Vector3D center = { 0.0, 0.0, 0.0 };
	double max_distance = 0.0;

	// Compute Center of Gravity
	for(int i = 0; i < num_particles; ++i) {
		center.x += particles[i]->position.x;
		center.y += particles[i]->position.y;
		center.z += particles[i]->position.z;
	}
	center.x /= num_particles;
	center.y /= num_particles;
	center.z /= num_particles;

	// Compute Max distance of the center of all directions
	for(int i = 0; i < num_particles; ++i) {
		double d = distance(particles[i]->position, center);
		if(d > max_distance) {
			max_distance = d;
		}
	}

	Node* root = create_node(center, 2.0 * max_distance);

	build_tree(particles, num_particles, root);

	for(int i = 0; i < num_particles; ++i) {
		update_force(particles[i], root);
		particles[i]->position.x += particles[i]->velocity.x;
		particles[i]->position.y += particles[i]->velocity.y;
		particles[i]->position.z += particles[i]->velocity.z;
	}

	free_tree(root);
}

int main(void) {
	Particle* particles[NUM_PARTICLES];
	FILE* file = fopen("data.dat", "w");
	if(!file) {
		printf("Error opening file\n");
		return EXIT_FAILURE;
	}

	srand(time(NULL));
	// Initialize particles with random values
	for(int i = 0; i < NUM_PARTICLES; ++i) {
		particles[i] = (Particle*)malloc(sizeof(Particle));
		particles[i]->mass = (double)rand() / RAND_MAX + 1;
		particles[i]->position.x = (double)rand() / RAND_MAX * 100;
		particles[i]->position.y = (double)rand() / RAND_MAX * 100;
		particles[i]->position.z = (double)rand() / RAND_MAX * 100;
		particles[i]->velocity.x = 0.0;
		particles[i]->velocity.y = 0.0;
		particles[i]->velocity.z = 0.0;
	}

	clock_t start = clock();
	// Run simulation
	for(int step = 0; step < NUM_STEPS; step++) {
		barnes_hut(particles, NUM_PARTICLES);
		for(int i = 0; i < NUM_PARTICLES; i++) {
			fprintf(file, "%f %f %f\n", particles[i]->position.x, particles[i]->position.y,
			        particles[i]->position.z);
		}
		fprintf(file, "\n\n");
	}

	clock_t end = clock();
	printf("Simulation time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);

	fclose(file);
	// Free allocated memory
	for(int i = 0; i < NUM_PARTICLES; i++) {
		free(particles[i]);
	}

	return EXIT_SUCCESS;
}

