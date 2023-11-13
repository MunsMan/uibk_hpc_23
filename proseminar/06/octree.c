#include "octree.h"
#include <stdio.h>
#include <stdlib.h>

Octree* init_tree(Vector3D max_position, Vector3D min_position) {

	Octree* octree = malloc(sizeof(Octree));
	octree->max_position = max_position;
	octree->min_position = min_position;
	octree->children = malloc(MAX_NUM_CHILDREN * sizeof(Octree*));
	for(int i = 0; i < MAX_NUM_CHILDREN; i++) {
		octree->children[i] = NULL;
	}
	octree->num_children = 0;
	octree->particle = NULL;
	return octree;
}
Octree* new_tree(uint8_t quad, Vector3D max_position, Vector3D min_position) {
	Octree* octree = malloc(sizeof(Octree));
	Vector3D min = {
		.x = (quad & 0b1) ? (max_position.x - min_position.x) / 2 + min_position.x : min_position.x,
		.y = (quad >> 1 & 0b1) ? (max_position.y - min_position.y) / 2 + min_position.y
		                       : min_position.y,
		.z = (quad >> 2 & 0b1) ? (max_position.z - min_position.z) / 2 + min_position.z
		                       : min_position.z,
	};
	Vector3D max = {
		.x = (quad & 0b1) ? max_position.x : (max_position.x - min_position.x) / 2 + min_position.x,
		.y = (quad >> 1 & 0b1) ? max_position.y
		                       : (max_position.y - min_position.y) / 2 + min_position.y,
		.z = (quad >> 2 & 0b1) ? max_position.z
		                       : (max_position.z - min_position.z) / 2 + min_position.z,
	};
	octree->min_position = min;
	octree->max_position = max;
	octree->children = malloc(MAX_NUM_CHILDREN * sizeof(Octree*));
	for(int i = 0; i < MAX_NUM_CHILDREN; i++) {
		octree->children[i] = NULL;
	}
	octree->num_children = 0;
	octree->particle = NULL;
	return octree;
}

uint8_t quadrant(Octree* octree, Particle* particle) {
	Vector3D center = {
		.x = (octree->max_position.x - octree->min_position.x) / 2 + octree->min_position.x,
		.y = (octree->max_position.y - octree->min_position.y) / 2 + octree->min_position.y,
		.z = (octree->max_position.z - octree->min_position.z) / 2 + octree->min_position.z,
	};
	return ((center.z > particle->position.z) << 2) + ((center.x > particle->position.x) << 1) +
	       (center.x > particle->position.x);
}

CenterGravity insert(Octree* octree, Particle* particle) {
	if(octree == NULL) {
		printf("Unable to insert on undefined Octree!");
		exit(EXIT_FAILURE);
	}
	if(octree->num_children == 0 && octree->particle == NULL) {
		octree->particle = particle;
		CenterGravity cg = { .position = particle->position, .mass = particle->mass };
		octree->center_gravity = cg;
		return cg;
	} else if(octree->num_children) {
		uint8_t quad1 = quadrant(octree, particle);
		octree->children[quad1] = new_tree(quad1, octree->max_position, octree->min_position);
		CenterGravity cg1 = insert(octree->children[quad1], particle);
		uint8_t quad2 = quadrant(octree, octree->particle);
		CenterGravity cg2;
		if(quad1 == quad2) {
			cg2 = insert(octree->children[quad1], octree->particle);
		} else {
			octree->children[quad2] = new_tree(quad2, octree->max_position, octree->min_position);
			cg2 = insert(octree->children[quad2], particle);
		}
		octree->num_children = 2;
		octree->particle = NULL;
		octree->center_gravity = add_cg(cg1, cg2);
		return octree->center_gravity;
	}
	uint8_t quad = quadrant(octree, particle);
	if(octree->children[quad] == NULL) {
		octree->children[quad] = new_tree(quad, octree->max_position, octree->min_position);
		octree->children++;
	}
	CenterGravity cg = insert(octree->children[quad], particle);
	octree->center_gravity = add_cg(cg, octree->center_gravity);
	return cg;
}

CenterGravity add_cg(CenterGravity cg1, CenterGravity cg2) {
	double mass = cg1.mass + cg2.mass;
	CenterGravity cg = { .position= {
		                     .x = (cg1.position.x * cg1.mass + cg2.position.x * cg2.mass) / mass,
		                     .y = (cg1.position.y * cg1.mass + cg2.position.y * cg2.mass) / mass,
		                     .z = (cg1.position.z * cg1.mass + cg2.position.z * cg2.mass) / mass,
		                 },
		                 .mass = mass };
	return cg;
}

int main(void) {
	Particle particles[8] = { { { .x = 1, .y = 1, .z = 1 }, { 0, 0, 0 }, 1 },
		                      { { .x = 1, .y = 1, .z = -1 }, { 0, 0, 0 }, 1 },
		                      { { .x = 1, .y = -1, .z = 1 }, { 0, 0, 0 }, 1 },
		                      { { .x = 1, .y = -1, .z = -1 }, { 0, 0, 0 }, 1 },
		                      { { .x = -1, .y = 1, .z = 1 }, { 0, 0, 0 }, 1 },
		                      { { .x = -1, .y = 1, .z = -1 }, { 0, 0, 0 }, 1 },
		                      { { .x = -1, .y = -1, .z = 1 }, { 0, 0, 0 }, 1 },
		                      { { .x = -1, .y = -1, .z = -1 }, { 0, 0, 0 }, 1 } };
	Vector3D max_position = { 1, 1, 1 };
	Vector3D min_position = { 0, 0, 0 };
	Octree* octree = init_tree(max_position, min_position);
	for(int i = 0; i < 8; i++) {
		insert(octree, &particles[i]);
	}
	return EXIT_SUCCESS;
}
