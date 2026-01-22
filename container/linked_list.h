#pragma once
#include <stdlib.h>

struct pindf_llist_node;

struct pindf_llist_node {
	void *v;
	size_t v_size;

	struct pindf_llist_node *n;
};

struct pindf_llist_node *pindf_llist_node_new();
void pindf_llist_node_init(struct pindf_llist_node *node);