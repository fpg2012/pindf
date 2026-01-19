#pragma once

#include "type.h"
#include <stdlib.h>
#include <string.h>

struct pindf_vector {
	uchar *buf;
	size_t len;
	size_t capacity;
	size_t elem_size;
	pindf_destroy_func destroy_func;
};

struct pindf_vector *pindf_vector_new(size_t capacity, size_t elem_size, pindf_destroy_func destroy_func);
void pindf_vector_init(struct pindf_vector *vec, size_t capacity, size_t elem_size, pindf_destroy_func destroy_func);
void pindf_vector_append(struct pindf_vector *vec, void *item);
void pindf_vector_pop(struct pindf_vector *vec, void *item);
void pindf_vector_last_elem(struct pindf_vector *vec, void *item);
void pindf_vector_destroy(struct pindf_vector *vec);
void pindf_vector_index(struct pindf_vector *vec, size_t index, void *item);