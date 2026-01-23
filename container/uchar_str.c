#include "uchar_str.h"

void pindf_uchar_str_init(pindf_uchar_str *str, size_t len)
{
	*str = (pindf_uchar_str){
		.capacity = len,
		.len = len,
		.p = (uchar*)malloc(len + 10),
	};
	memset(str->p, 0x00, len);
}

pindf_uchar_str *pindf_uchar_str_new()
{
	pindf_uchar_str *str = (pindf_uchar_str*)malloc(sizeof(pindf_uchar_str));
	return str;
}

pindf_uchar_str *pindf_uchar_str_from_cstr(const char *str, size_t len)
{
	pindf_uchar_str *ustr = pindf_uchar_str_new();
	pindf_uchar_str_init(ustr, len);
	memcpy(ustr->p, str, len);
	return ustr;
}

void pindf_uchar_str_init_w_p(pindf_uchar_str *str, uchar *p, int len)
{
	str->p = p;
	len = len;
}

void pindf_uchar_str_expand(pindf_uchar_str *str, size_t new_size)
{
	str->p = realloc(str->p, new_size);
	memset(str->p + str->capacity, 0x00, new_size - str->capacity);
	str->capacity = new_size;
}

void pindf_uchar_str_2xexpand(pindf_uchar_str *str)
{
	pindf_uchar_str_expand(str, 2 * str->capacity);
}

void pindf_uchar_str_destroy(pindf_uchar_str *str)
{
	free(str->p);
	free(str);
}

void pindf_uchar_str_destroy_wo_p(pindf_uchar_str *str)
{
	free(str);
}

int pindf_uchar_str_cmp(pindf_uchar_str *a, pindf_uchar_str *b)
{
	assert(a != NULL);
	assert(b != NULL);
	uchar *p = a->p, *q = b->p;
	uchar *p_end = a->p + a->len, *q_end = q + b->len;
	while (p != p_end && q != q_end) {
		if (*p < *q) {
			return -1;
		} else if (*p > *q) {
			return 1;
		}
		++p, ++q;
	}
	if (a->len < b->len) {
		return -1;
	} else if (a->len > b->len) {
		return 1;
	}
	return 0;
}