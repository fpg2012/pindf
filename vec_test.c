#include <stdio.h>
#include "container/simple_vector.h"

int main() {
	struct pindf_vector *vec = pindf_vector_new(10, sizeof(int), NULL);
	
	int temp = 0;
	for (int i = 0; i < 10; ++i) {
		temp = 100 + i;
		pindf_vector_append(vec, &temp);
	}

	temp = 10001;

	while (vec->len > 0) {
		pindf_vector_pop(vec, &temp);
		printf("%d\n", temp);
	}
}