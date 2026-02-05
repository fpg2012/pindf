#pragma once

#include "../type.h"
#include <stdlib.h>
#include <string.h>

/// @brief simple general vector struct
/// note that vector should own its direct elements
/// the vector do not own the refernced items if elements are pointers
typedef struct {
	/// underlying memory space
	uchar *buf;
	/// length of vector
	size_t len;
	/// capacity of vector (real lenght of underlying buffer)
	size_t capacity;
	/// size of element
	size_t elem_size;
} pindf_vector;

pindf_vector *pindf_vector_new(size_t capacity, size_t elem_size);
void pindf_vector_init(pindf_vector *vec, size_t capacity, size_t elem_size);

/// @brief append an item to the vector
/// copy `elem_size` bytes from the memory referenced by item to buf
/// @param item pointer to the memory space of input item
void pindf_vector_append(pindf_vector *vec, void *item);

/// @brief return and remove the last item from vector
/// copy the last item to the mem space referenced by item unless it is NULL.
/// and then decrease vec->len
/// @param item pointer to the memory space to store the last item.
void pindf_vector_pop(pindf_vector *vec, void *item);

/// @brief return the last item from vector
/// copy the last item to the mem space referenced by item.
/// @param item pointer to the memory space to store the last item. shuold not be NULL
void pindf_vector_last_elem(pindf_vector *vec, void *item);

/// free the space of buf
void pindf_vector_destroy(pindf_vector *vec);

/// @brief get `index`th item from vector
void pindf_vector_index(pindf_vector *vec, size_t index, void *item);