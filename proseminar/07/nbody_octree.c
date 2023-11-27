#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define G 1        // 6.67430e-11 // gravitational constant
#define THETA 0.55 // Efficency Parameter
#define IMBALANCE_FACTOR 0.5

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

typedef struct PreAlocNodes {
	int num_nodes;
	int used_nodes;
	Node* nodes;
} PreAlocNodes;

Node* create_node(Vector3D center, double size, PreAlocNodes* pre_aloc_nodes) {
	Node* node = &pre_aloc_nodes->nodes[pre_aloc_nodes->used_nodes++];
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

void print_tree(FILE* file, Node* node, int depth) {
	for(int i = 0; i < depth; i++) {
		fprintf(file, "\t");
	}
	int not_leaf = 0;
	for(int i = 0; i < 8; i++) {
		if(node->children[i]) {
			not_leaf++;
			break;
		}
	}
	if(not_leaf) {
		fprintf(file, "Node: %f %f %f\n", node->center_of_mass.x, node->center_of_mass.y,
		        node->center_of_mass.z);
	} else {
		fprintf(file, "Leaf: %f %f %f\n", node->center_of_mass.x, node->center_of_mass.y,
		        node->center_of_mass.z);
	}
	for(int i = 0; i < 8; i++) {
		if(node->children[i]) {
			print_tree(file, node->children[i], depth + 1);
		}
	}
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

void build_tree(Particle* particles[], int num_particles, Node* node,
                PreAlocNodes* pre_aloc_nodes) {
	if(num_particles == 1) {
		node->mass = particles[0]->mass;
		node->center_of_mass.x = particles[0]->position.x;
		node->center_of_mass.y = particles[0]->position.y;
		node->center_of_mass.z = particles[0]->position.z;
		return;
	}

	for(int i = 0; i < 8; i++) {
		Vector3D sub_center;
		sub_center.x = node->center.x + 0.5 * node->size * ((i & 1 << (2)) ? 1 : -1);
		sub_center.y = node->center.y + 0.5 * node->size * ((i & 1 << (1)) ? 1 : -1);
		sub_center.z = node->center.z + 0.5 * node->size * ((i & 1 << (0)) ? 1 : -1);

		int num_sub_particles = 0;
		Particle* sub_particles[num_particles];
		for(int j = 0; j < num_particles; j++) {
			if(fabs(particles[j]->position.x - sub_center.x) < 0.5 * node->size &&
			   fabs(particles[j]->position.y - sub_center.y) < 0.5 * node->size &&
			   fabs(particles[j]->position.z - sub_center.z) < 0.5 * node->size) {
				sub_particles[num_sub_particles++] = particles[j];
			}
		}
		if(num_sub_particles > 0) {
			node->children[i] = create_node(sub_center, 0.5 * node->size, pre_aloc_nodes);
			build_tree(sub_particles, num_sub_particles, node->children[i], pre_aloc_nodes);
		}
	}

	for(int i = 0; i < 8; i++) {
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

void barnes_hut(Particle* particles[], int num_particles, int num_ranks, int my_rank,
                PreAlocNodes* pre_aloc_nodes) {
	pre_aloc_nodes->used_nodes = 0;
	Vector3D center = { 0.0, 0.0, 0.0 };
	double max_distance = 0.0;

	// Compute Center of Gravity
	for(int i = 0; i < num_particles; ++i) {
		center.x += particles[i]->position.x;
		center.y += particles[i]->position.y;
		center.z += particles[i]->position.z;
		// Compute Max distance of the center of all directions
		double d = distance(particles[i]->position, center);
		if(d > max_distance) {
			max_distance = d;
		}
	}
	center.x /= num_particles;
	center.y /= num_particles;
	center.z /= num_particles;

	Node* root = create_node(center, 2.0 * max_distance, pre_aloc_nodes);

	build_tree(particles, num_particles, root, pre_aloc_nodes);

	int step_width = num_particles / num_ranks;
	int rank_range =
	    (step_width * (my_rank + 1) < num_particles ? step_width * (my_rank + 1) : num_particles);
	for(int i = step_width * my_rank; i < rank_range; i++) {
		update_force(particles[i], root);
		particles[i]->position.x += particles[i]->velocity.x;
		particles[i]->position.y += particles[i]->velocity.y;
		particles[i]->position.z += particles[i]->velocity.z;
	}
}

int main(int argc, char* argv[]) {
	FILE* file = fopen("data.dat", "w");
	if(!file) {
		printf("Error opening file\n");
		return EXIT_FAILURE;
	}

	srand(time(NULL));
	int my_rank, num_ranks;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

	if(argc < 3) {
		printf("Please run with: ./nbody <numParticles> <iterations>\n");
		exit(1);
	}

	int numParticles = atoi(argv[1]);
	int numSteps = atoi(argv[2]);

	Particle* particles = malloc(numParticles * sizeof(Particle));
	Particle** particles_pointer = malloc(numParticles * sizeof(Particle*));
	Node* nodes = malloc(numParticles * 2 * sizeof(Node));
	PreAlocNodes pre_aloc_nodes = { .nodes = nodes,
		                            .num_nodes = numParticles * 2,
		                            .used_nodes = 0 };

	MPI_Request requests[num_ranks];
	MPI_Datatype MPI_PARTICLE;
	MPI_Type_contiguous(sizeof(Particle), MPI_BYTE, &MPI_PARTICLE);
	MPI_Type_commit(&MPI_PARTICLE);

	// Initialize particles with random values
	for(int i = 0; i < numParticles; i++) {
		particles_pointer[i] = &particles[i];
		particles[i].mass = (double)rand() / RAND_MAX + 1;
		// particles[i].position.x = (double)rand() / RAND_MAX * 100;
		// particles[i].position.y = (double)rand() / RAND_MAX * 100;
		// particles[i].position.z = (double)rand() / RAND_MAX * 100;

		// Spacially imbalanced initial conditions
		double imbalanceFactor = (double)rand() / RAND_MAX;
		if(imbalanceFactor < IMBALANCE_FACTOR) {
			particles[i].position.x = (double)rand() / RAND_MAX * 50;
			particles[i].position.y = (double)rand() / RAND_MAX * 50;
			particles[i].position.z = (double)rand() / RAND_MAX * 50;
		} else {
			particles[i].position.x = 50 + (double)rand() / RAND_MAX * 50;
			particles[i].position.y = 50 + (double)rand() / RAND_MAX * 50;
			particles[i].position.z = 50 + (double)rand() / RAND_MAX * 50;
		}
		particles[i].velocity.x = 0.0;
		particles[i].velocity.y = 0.0;
		particles[i].velocity.z = 0.0;
	}
	int step_width = numParticles / num_ranks;
	int size = my_rank = !num_ranks - 1 ? step_width : numParticles - (num_ranks - 1) * step_width;
	for(int i = 0; i < num_ranks - 1; i++) {

		MPI_Ibcast(&particles[i * step_width], step_width, MPI_PARTICLE, i, MPI_COMM_WORLD,
		           &requests[num_ranks - 1]);
	}
	MPI_Ibcast(&particles[(num_ranks - 1) * step_width],
	           numParticles - (num_ranks - 1) * step_width, MPI_PARTICLE, num_ranks - 1,
	           MPI_COMM_WORLD, &requests[num_ranks - 1]);
	MPI_Barrier(MPI_COMM_WORLD);
	double start;
	start = MPI_Wtime();
	if(my_rank == 0) {
		printf("Number of Rangs: %d with %d Particles and %d Iterations\n", num_ranks, numParticles,
		       numSteps);
	}
	// Run simulation
	for(int step = 0; step < numSteps; step++) {
		barnes_hut(particles_pointer, numParticles, num_ranks, my_rank, &pre_aloc_nodes);
		for(int i = 0; i < num_ranks - 1; i++) {
			MPI_Ibcast(&particles[i * step_width], step_width, MPI_PARTICLE, i, MPI_COMM_WORLD,
			           &requests[num_ranks - 1]);
		}
		MPI_Ibcast(&particles[(num_ranks - 1) * step_width],
		           numParticles - (num_ranks - 1) * step_width, MPI_PARTICLE, num_ranks - 1,
		           MPI_COMM_WORLD, &requests[num_ranks - 1]);
		MPI_Barrier(MPI_COMM_WORLD);
	}

	double end = MPI_Wtime();
	if(my_rank == 0) {
		printf("Simulation time: %f seconds\n", end - start);
	}
	MPI_Type_free(&MPI_PARTICLE);
	MPI_Finalize();

	free(particles_pointer);
	free(particles);
	free(nodes);
	fclose(file);
	return EXIT_SUCCESS;
}
