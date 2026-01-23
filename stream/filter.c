#include "filter.h"
#include <zlib.h>

/*
note

stream should be use like a wrapper, should not be loaded into memory entirely

stream_getc(stream)
*/

int pindf_zlib_uncompress(pindf_uchar_str *dest, pindf_uchar_str *src)
{
	size_t cap = dest->capacity;
	int ret = 0;
	int n_retry = 0;
	do {
		ret = uncompress(dest->p, &cap, src->p, src->len);
		if (ret == Z_MEM_ERROR) {
			if (n_retry >= PINDF_MAX_EXPAND_RETRY) {
				fprintf(stderr, "[error] used up 2^%d times of mem, still not enough!", PINDF_MAX_EXPAND_RETRY);
				ret = Z_DATA_ERROR;
				break;
			}
			pindf_uchar_str_2xexpand(dest);
		}
	} while(ret == Z_MEM_ERROR);
	
	if (ret == Z_DATA_ERROR)
		return PINDF_FLTR_DAT_ERR;
	if (ret == Z_MEM_ERROR)
		return PINDF_FLTR_MEM_ERR;

	dest->len = cap;
	return cap; 
}


int pindf_zlib_compress(pindf_uchar_str *dest, pindf_uchar_str *src)
{
	size_t cap = dest->capacity;
	int ret = 0;
	int n_retry = 0;
	do {
		ret = compress(dest->p, &cap, src->p, src->len);
		if (ret == Z_MEM_ERROR) {
			if (n_retry >= PINDF_MAX_EXPAND_RETRY) {
				fprintf(stderr, "[error] used up 2^%d times of mem, still not enough!", PINDF_MAX_EXPAND_RETRY);
				break;
			}
			pindf_uchar_str_2xexpand(dest);
		}
	} while(ret == Z_MEM_ERROR);
	
	if (ret == Z_DATA_ERROR)
		return PINDF_FLTR_DAT_ERR;
	if (ret == Z_MEM_ERROR)
		return PINDF_FLTR_MEM_ERR;

	dest->len = cap;
	return cap; 
}