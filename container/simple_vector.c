#include "simple_vector.h"
#include <stdlib.h>
#include <assert.h>

pindf_vector *pindf_vector_new(size_t capacity, size_t elem_size)
{
	pindf_vector *vec = (pindf_vector *)malloc(sizeof(pindf_vector));
	pindf_vector_init(vec, capacity, elem_size);
	return vec;
}

void pindf_vector_init(pindf_vector *vec, size_t capacity, size_t elem_size)
{
	vec->buf = (uchar*)calloc(capacity + 1, elem_size);
	vec->len = 0;
	vec->capacity = capacity;
	vec->elem_size = elem_size;
}

void pindf_vector_append(pindf_vector *vec, void *item)
{
	assert(item != NULL);

	if (vec->len == vec->capacity) {
		vec->buf = (uchar*)realloc(vec->buf, (vec->capacity*2 + 1) * vec->elem_size);
		vec->capacity *= 2;
	}

	memcpy(vec->buf + vec->len*vec->elem_size, item, vec->elem_size);
	vec->len++;
}

void pindf_vector_pop(pindf_vector *vec, void *item)
{
	assert(vec->len > 0);
	--vec->len;
	if (item != NULL) {
		memcpy(item, vec->buf + (vec->len)*vec->elem_size, vec->elem_size);
	}
}

void pindf_vector_destroy(pindf_vector *vec)
{
	free(vec->buf);
}

void pindf_vector_last_elem(pindf_vector *vec, void *item)
{
	assert(vec->len > 0);
	assert(item != NULL);

	memcpy(item, vec->buf + (vec->len - 1)*vec->elem_size, vec->elem_size);
}

void pindf_vector_index(pindf_vector *vec, size_t index, void *item)
{
	assert(vec->len > 0);
	assert(item != NULL);

	memcpy(item, vec->buf + index*vec->elem_size, vec->elem_size);
}