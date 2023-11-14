#include "octree.h"
#include <stdio.h>

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
Octree* new_tree(uint8_t quad, Vector3D max_position, Vector3D min_position, Octree* parent) {
	Octree* octree = malloc(sizeof(Octree));
	if(!octree) {
		printf("Malloc failed\n");
		exit(EXIT_FAILURE);
	}
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
	octree->parent = parent;
	return octree;
}

uint8_t quadrant(Octree* octree, Particle* particle) {
	Vector3D center = {
		.x = (octree->max_position.x - octree->min_position.x) / 2 + octree->min_position.x,
		.y = (octree->max_position.y - octree->min_position.y) / 2 + octree->min_position.y,
		.z = (octree->max_position.z - octree->min_position.z) / 2 + octree->min_position.z,
	};
	return ((center.z < particle->position.z) << 2) + ((center.y < particle->position.y) << 1) +
	       (center.x < particle->position.x);
}

CenterGravity insert(Octree* octree, Particle* particle) {
	if(!octree) {
		printf("Unable to insert on undefined Octree!");
		exit(EXIT_FAILURE);
	}
	if(octree->num_children == 0 && !octree->particle) {
		octree->particle = particle;
		CenterGravity cg = { .position = particle->position, .mass = particle->mass };
		octree->center_gravity = cg;
		return cg;
	} else if(octree->num_children == 0) {
		uint8_t quad1 = quadrant(octree, particle);
		octree->children[quad1] =
		    new_tree(quad1, octree->max_position, octree->min_position, octree);
		CenterGravity cg1 = insert(octree->children[quad1], particle);
		uint8_t quad2 = quadrant(octree, octree->particle);
		CenterGravity cg2;
		if(quad1 == quad2) {
			cg2 = insert(octree->children[quad1], octree->particle);
		} else {
			octree->children[quad2] =
			    new_tree(quad2, octree->max_position, octree->min_position, octree);
			cg2 = insert(octree->children[quad2], octree->particle);
		}
		octree->num_children = 2;
		octree->particle = NULL;
		octree->center_gravity = add_cg(cg1, cg2);
		return octree->center_gravity;
	} else {
		uint8_t quad = quadrant(octree, particle);
		if(!octree->children[quad]) {
			octree->children[quad] =
			    new_tree(quad, octree->max_position, octree->min_position, octree);
			octree->num_children++;
		}

		CenterGravity cg = insert(octree->children[quad], particle);
		octree->center_gravity = add_cg(cg, octree->center_gravity);
		return cg;
	}
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

double cg_effect(CenterGravity cg1, CenterGravity cg2) {
	Vector3D diff = vec_sub(cg1.position, cg2.position);
	double len = vec_length(diff);
	return (cg1.mass + cg2.mass) / len;
}

Vector3D vec_sub(Vector3D v1, Vector3D v2) {
	Vector3D res = {
		.x = v1.x - v2.x,
		.y = v1.y - v2.y,
		.z = v1.z - v2.z,
	};
	return res;
}
Vector3D vec_add(Vector3D v1, Vector3D v2) {
	Vector3D res = {
		.x = v1.x + v2.x,
		.y = v1.y + v2.y,
		.z = v1.z + v2.z,
	};
	return res;
}

double vec_length(Vector3D v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

void print_tree(Octree* octree, int depth) {
	for(int i = 0; i < depth; i++) {
		printf("\t");
	}
	printf("Node - %p - (%f,%f,%f)\n", octree, octree->center_gravity.position.x,
	       octree->center_gravity.position.y, octree->center_gravity.position.z);
	if(octree->num_children) {
		for(int i = 0; i < depth; i++) {
			printf("\t");
		}
		printf("Children:\n");
		for(int i = 0; i < 8; i++) {
			if(octree->children[i]) {
				print_tree(octree->children[i], depth + 1);
			}
		}
	}
}

bool is_particle(Octree* octree) {
	return !(!octree->particle);
}

Octree* down(Octree* octree) {
	if(is_particle(octree)) {
		return octree;
	}
	for(int i = 0; i < MAX_NUM_CHILDREN; i++) {
		if(octree->children[i]) {
			return down(octree->children[i]);
		}
	}
	return NULL;
};

Octree* next(Octree* octree) {
	if(!octree->parent) {
		return NULL;
	}
	bool found_self = false;
	for(int i = 0; i < MAX_NUM_CHILDREN; i++) {
		if(octree->parent->children[i] == octree) {
			found_self = true;
			continue;
		}
		if(found_self && octree->parent->children[i]) {
			return down(octree->parent->children[i]);
		}
	};
	return next(octree->parent);
}

IteratorState* iterator_leaf_init(Octree* octree) {
	IteratorState* state = malloc(sizeof(IteratorState));
	state->position = down(octree);
	return state;
}

Octree* iterator_leaf_next(IteratorState* state) {
	Octree* current = state->position;
	if(current) {
		state->position = next(current);
	}
	return current;
}

bool iterator_leaf_has_next(IteratorState* state) {
	return state->position;
}

List* init_list() {
	List* list = malloc(sizeof(List));
	list->size = 0;
	list->first = NULL;
	list->last = NULL;
	return list;
}

ListNode* init_node(void* element) {
	ListNode* node = malloc(sizeof(ListNode));
	node->element = element;
	return node;
}

List* append(List* list, void* element) {
	ListNode* new_node = init_node(element);
	if(list->size == 0) {
		list->first = new_node;
	} else {
		list->last->next = new_node;
		new_node->prev = list->last;
	}
	list->last = new_node;
	list->size += 1;
	return list;
}

List* concat(List* list1, List* list2) {
	if(list1->size == 0) {
		free(list1);
		return list2;
	}
	if(list2->size == 0) {
		free(list2);
		return list1;
	}
	list1->last->next = list2->first;
	list1->last = list2->last;
	list1->size += list2->size;
	free(list2);
	return list1;
}

void list_free(List* list) {
	ListNode* node = list->first;
	while(node) {
		node = node->next;
		free(node->prev);
	}
	free(list);
}

Octree* move_up(Octree* octree, Octree* origin, double threshold_effect) {
	if(!octree->parent) {
		return octree;
	}
	double effect = cg_effect(origin->center_gravity, octree->parent->center_gravity);
	if(effect < threshold_effect) {
		return octree->parent;
	}
	return move_up(octree->parent, origin, threshold_effect);
}

List* collect_down(Octree* octree, Octree* origin, double threshold_effect) {
	List* list = init_list();
	double effect = cg_effect(origin->center_gravity, octree->center_gravity);
	if(is_particle(octree)) {
		list = append(list, (void*)&octree->center_gravity);
		return list;
	}

	if(effect < threshold_effect) {
		list = append(list, (void*)&octree->center_gravity);
		return list;
	}
	for(int i = 0; i < MAX_NUM_CHILDREN; i++) {
		if(!octree->children[i]) {
			continue;
		}
		list = concat(list, collect_down(octree->children[i], origin, threshold_effect));
	}
	return list;
}

List* effecting_neigbours(Octree* octree, double threshold_effect) {
	if(!octree->parent) {
		printf("ROOT has no effecting neigbhours");
		exit(EXIT_FAILURE);
	}
	Octree* upperBound = move_up(octree->parent, octree, threshold_effect);
	return collect_down(upperBound, octree, threshold_effect);
}

int main(void) {
	Particle particles[12] = { { { .x = 1, .y = 1, .z = 1 }, { 0, 0, 0 }, 1 },
		                       { { .x = 1, .y = 1, .z = 0 }, { 0, 0, 0 }, 1 },
		                       { { .x = 1, .y = 1, .z = -1 }, { 0, 0, 0 }, 1 },
		                       { { .x = 1, .y = -1, .z = 1 }, { 0, 0, 0 }, 1 },
		                       { { .x = 1, .y = -1, .z = 0 }, { 0, 0, 0 }, 1 },
		                       { { .x = 1, .y = -1, .z = -1 }, { 0, 0, 0 }, 1 },
		                       { { .x = -1, .y = 1, .z = 1 }, { 0, 0, 0 }, 1 },
		                       { { .x = -1, .y = 1, .z = 0 }, { 0, 0, 0 }, 1 },
		                       { { .x = -1, .y = 1, .z = -1 }, { 0, 0, 0 }, 1 },
		                       { { .x = -1, .y = -1, .z = 1 }, { 0, 0, 0 }, 1 },
		                       { { .x = -1, .y = -1, .z = 0 }, { 0, 0, 0 }, 1 },
		                       { { .x = -1, .y = -1, .z = -1 }, { 0, 0, 0 }, 1 } };
	Vector3D max_position = { 1, 1, 1 };
	Vector3D min_position = { -1, -1, -1 };
	Octree* octree = init_tree(max_position, min_position);
	for(int i = 0; i < 12; i++) {
		insert(octree, &particles[i]);
	}
	print_tree(octree, 0);
	printf("\n\n");
	IteratorState* state = iterator_leaf_init(octree);
	printf("%p\n", state->position);
	while(iterator_leaf_has_next(state)) {
		Octree* node = iterator_leaf_next(state);
		printf("Node - %p - (%f,%f,%f)\n", node, node->center_gravity.position.x,
		       node->center_gravity.position.y, node->center_gravity.position.z);
		List* list = effecting_neigbours(node, 1.5);
		printf("%d\n", list->size);
	}
	return EXIT_SUCCESS;
}
