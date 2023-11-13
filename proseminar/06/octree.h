#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_NUM_CHILDREN 8

typedef struct {
	double x, y, z;
} Vector3D;

typedef struct {
	Vector3D position;
	Vector3D velocity;
	double mass;
} Particle;

typedef struct {
	Vector3D position;
	double mass;
} CenterGravity;

typedef struct Octree_t {
	Vector3D max_position;
	Vector3D min_position;
	uint8_t num_children;
	struct Octree_t** children;
	Particle* particle;
	CenterGravity center_gravity;
} Octree;

Octree* new_tree(uint8_t quad, Vector3D max_position, Vector3D min_position);
Octree* init_tree(Vector3D max_position, Vector3D min_position);
CenterGravity insert(Octree* octree, Particle* particle);
uint8_t quadrant(Octree* octree, Particle* particle);
CenterGravity add_cg(CenterGravity cg1, CenterGravity cg2);
