#pragma once

#include "../type.h"
#include <string.h>
#include <stdlib.h>

/// @brief byte string
typedef struct {
	uchar *p;
	size_t len;
	size_t capacity;
} pindf_uchar_str;


void pindf_uchar_str_init(pindf_uchar_str *str, size_t len);
pindf_uchar_str *pindf_uchar_str_new();
pindf_uchar_str *pindf_uchar_str_from_cstr(const char *str, size_t len);

void pindf_uchar_str_destroy(pindf_uchar_str *str);
void pindf_uchar_str_destroy_wo_p(pindf_uchar_str *str);

/// @brief strcmp-like uchar string comparison
/// a < b -> -1
/// a == b -> 0
/// a > b -> 1
int pindf_uchar_str_cmp(const pindf_uchar_str *a, const pindf_uchar_str *b);
int pindf_uchar_str_cmp2(const pindf_uchar_str *a, const uchar *b, size_t len);

/// @brief strcmp-like uchar string comparison
/// @param b c string for comparison
int pindf_uchar_str_cmp3(const pindf_uchar_str *a, const char *b);

/// @brief expand underlying memory space to new_size
/// panic if failed
/// @param new_size target size for expansion
void pindf_uchar_str_expand(pindf_uchar_str *str, size_t new_size);

/// @brief double the size of underlying memory space
/// panic if failed
void pindf_uchar_str_2xexpand(pindf_uchar_str *str);