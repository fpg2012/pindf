#pragma once

#include "../type.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
	uchar *p;
	size_t len;
} pindf_uchar_str;


void pindf_uchar_str_init(pindf_uchar_str *str, size_t len);
pindf_uchar_str *pindf_uchar_str_new();
void pindf_uchar_str_destroy(pindf_uchar_str *str);
void pindf_uchar_str_destroy_wo_p(pindf_uchar_str *str);