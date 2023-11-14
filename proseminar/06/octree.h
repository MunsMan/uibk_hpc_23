#include <math.h>
#include <stdbool.h>
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
	struct Octree_t* parent;
} Octree;

Octree* new_tree(uint8_t quad, Vector3D max_position, Vector3D min_position, Octree* parent);
Octree* init_tree(Vector3D max_position, Vector3D min_position);
CenterGravity insert(Octree* octree, Particle* particle);
uint8_t quadrant(Octree* octree, Particle* particle);
void print_tree(Octree* octree, int depth);
bool is_particle(Octree* octree);
void free_tree(Octree* octree);

CenterGravity add_cg(CenterGravity cg1, CenterGravity cg2);
double cg_effect(CenterGravity cg1, CenterGravity cg2);
CenterGravity get_cg(Octree* octree);

Vector3D vec_sub(Vector3D v1, Vector3D v2);
Vector3D vec_add(Vector3D v1, Vector3D v2);
double vec_length(Vector3D v);

typedef struct {
	Octree* position;
} IteratorState;

IteratorState* iterator_leaf_init(Octree* octree);
Octree* iterator_leaf_next(IteratorState* state);
bool iterator_leaf_has_next(IteratorState* state);

typedef struct ListNode_t {
	struct ListNode_t* next;
	struct ListNode_t* prev;
	void* element;
} ListNode;

typedef struct {
	int size;
	ListNode* first;
	ListNode* last;
} List;

List* init_list(void);
ListNode* init_node(void* element);
List* append(List* list, void* element);
List* concat(List* list1, List* list2);
void list_free(List* list);

List* effecting_neigbours(Octree* octree, double threshold_effect);
