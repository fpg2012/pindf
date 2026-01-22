#pragma once

#include "../type.h"
#include <string.h>
#include <stdlib.h>

struct pindf_uchar_str {
	uchar *p;
	size_t len;
};


void pindf_uchar_str_init(struct pindf_uchar_str *str, size_t len);
struct pindf_uchar_str *pindf_uchar_str_new();
void pindf_uchar_str_destroy(struct pindf_uchar_str *str);
void pindf_uchar_str_destroy_wo_p(struct pindf_uchar_str *str);