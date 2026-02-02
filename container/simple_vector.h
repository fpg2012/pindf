#pragma once

#include "../type.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	uchar *buf;
	size_t len;
	size_t capacity;
	size_t elem_size;
} pindf_vector;

pindf_vector *pindf_vector_new(size_t capacity, size_t elem_size);
void pindf_vector_init(pindf_vector *vec, size_t capacity, size_t elem_size);
void pindf_vector_append(pindf_vector *vec, void *item);
void pindf_vector_pop(pindf_vector *vec, void *item);
void pindf_vector_last_elem(pindf_vector *vec, void *item);
void pindf_vector_destroy(pindf_vector *vec);
void pindf_vector_index(pindf_vector *vec, size_t index, void *item);