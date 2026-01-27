#pragma once

#include "../type.h"
#include <string.h>
#include <stdlib.h>

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
int pindf_uchar_str_cmp(pindf_uchar_str *a, pindf_uchar_str *b);
int pindf_uchar_str_cmp2(pindf_uchar_str *a, const uchar *b, size_t len);
void pindf_uchar_str_expand(pindf_uchar_str *str, size_t new_size);
void pindf_uchar_str_2xexpand(pindf_uchar_str *str);