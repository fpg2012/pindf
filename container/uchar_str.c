#include "uchar_str.h"

void pindf_uchar_str_init(pindf_uchar_str *str, size_t len)
{
	str->p = (uchar*)malloc(len + 10);
	str->len = len;
	memset(str->p, 0x00, len);
}

pindf_uchar_str *pindf_uchar_str_new()
{
	pindf_uchar_str *str = (pindf_uchar_str*)malloc(sizeof(pindf_uchar_str));
	return str;
}

void pindf_uchar_str_init_w_p(pindf_uchar_str *str, uchar *p, int len)
{
	str->p = p;
	len = len;
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