#include "uchar_str.h"

void pindf_uchar_str_init(struct pindf_uchar_str *str, size_t len)
{
	str->p = (uchar*)malloc(len + 10);
	str->len = len;
	memset(str->p, 0x00, len);
}

struct pindf_uchar_str *pindf_uchar_str_new()
{
	struct pindf_uchar_str *str = (struct pindf_uchar_str*)malloc(sizeof(struct pindf_uchar_str));
	return str;
}

void pindf_uchar_str_init_w_p(struct pindf_uchar_str *str, uchar *p, int len)
{
	str->p = p;
	len = len;
}

void pindf_uchar_str_destroy(struct pindf_uchar_str *str)
{
	free(str->p);
	free(str);
}

void pindf_uchar_str_destroy_wo_p(struct pindf_uchar_str *str)
{
	free(str);
}