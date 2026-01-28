#pragma once
#include <assert.h>

#define PINDF_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PINDF_MAX(a, b) ((a) > (b) ? (a) : (b))

typedef unsigned char uchar;
typedef unsigned int uint;
typedef long long int64;
typedef unsigned long long uint64;
typedef void (*pindf_destroy_func)(void *);

enum pindf_errno {
	PINDF_OK = 0,
	PINDF_EOF_ERR = -1,
	PINDF_GNR_ERR = -2,
	PINDF_MEM_ERR = -3
};