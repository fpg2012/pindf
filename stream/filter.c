#include "filter.h"
#include "../logger/logger.h"

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
				PINDF_ERR("used up 2^%d times of mem, still not enough!", PINDF_MAX_EXPAND_RETRY);
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
				PINDF_ERR("used up 2^%d times of mem, still not enough!", PINDF_MAX_EXPAND_RETRY);
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

int pindf_filter_init(pindf_stream_filter *filter, enum pindf_filter_type type, pindf_pdf_dict *decode_params)
{
	filter->type = type;

	if (type == PINDF_FLTR_TYPE_NONE) {
		filter->decode = NULL;
		filter->encode = NULL;
		return 0;
	} else if (type != PINDF_FLTR_TYPE_FLATEDECODE) {
		PINDF_ERR("filter type %d not implemented yet!", type);
		return -1;
	} else {
		filter->decode = pindf_flate_decode;
		filter->encode = pindf_flate_encode;

		int predictor = PINDF_FLTR_FLATE_PREDICTOR_NONE, colors = 1, bits_per_component = 8, columns = 1;
		if (decode_params != NULL) {
			pindf_pdf_obj *temp_obj;
			temp_obj = pindf_dict_getvalue2(decode_params, "/Predictor");
			if (temp_obj != NULL && temp_obj->obj_type == PINDF_PDF_INT) {
				predictor = (enum pindf_filter_flate_predictor)temp_obj->content.num;
			}

			if (predictor > 1) {
				temp_obj = pindf_dict_getvalue2(decode_params, "/Colors");
				if (temp_obj != NULL && temp_obj->obj_type == PINDF_PDF_INT) {
					colors = (int)temp_obj->content.num;
				}

				temp_obj = pindf_dict_getvalue2(decode_params, "/BitsPerComponent");
				if (temp_obj != NULL && temp_obj->obj_type == PINDF_PDF_INT) {
					bits_per_component = (int)temp_obj->content.num;
				}

				temp_obj = pindf_dict_getvalue2(decode_params, "/Columns");
				if (temp_obj != NULL && temp_obj->obj_type == PINDF_PDF_INT) {
					columns = (int)temp_obj->content.num;
				}
			}
		}

		filter->decode_params = (pindf_filter_decode_params){
			.predictor = predictor,
			.colors = colors,
			.bits_per_component = bits_per_component,
			.columns = columns,
		};
	}
	
	return 0;
}

int pindf_flate_decode(pindf_stream_filter *f, pindf_uchar_str *dest, pindf_uchar_str *src)
{
	if (f->type != PINDF_FLTR_TYPE_FLATEDECODE) {
		PINDF_ERR("filter type %d not supported in flate decode!", f->type);
		return PINDF_FLTR_DAT_ERR;
	}

	if (f->decode_params.predictor == PINDF_FLTR_FLATE_PREDICTOR_NONE) {
		return pindf_zlib_uncompress(dest, src);
	}
	
	if (f->decode_params.predictor >= 10 && f->decode_params.predictor <= 15) {
		pindf_uchar_str temp_dest;
		pindf_uchar_str_init(&temp_dest, dest->capacity);

		// PNG prediction
		int ret = pindf_zlib_uncompress(&temp_dest, src);
		if (ret < 0) {
			pindf_uchar_str_destroy(&temp_dest);
			return ret;
		}

		// Apply PNG prediction
		size_t sample_size = (f->decode_params.bits_per_component * f->decode_params.colors + 7) / 8;
		size_t columns = f->decode_params.columns;
		size_t row_size = sample_size * columns;
		size_t n_rows = temp_dest.len / (row_size + 1); // +1 for filter byte

		uchar buffer[row_size + 1];
		memset(buffer, 0, sizeof(buffer));

		uchar *p = temp_dest.p;
		uchar *q = dest->p;

		for (int i = 0; i < n_rows; ++i) {
			enum pindf_png_filter_type filter_type = *p++;
			for (int j = 1; j <= row_size; ++j, ++p) {
				uchar cur_byte = *p;
				
				switch (filter_type) {
				case PINDF_PNG_FLTR_TYPE_NONE:
					cur_byte = *p;
					break;
				case PINDF_PNG_FLTR_TYPE_SUB:
					cur_byte = *p + buffer[j - 1];
					break;
				case PINDF_PNG_FLTR_TYPE_UP:
					cur_byte = *p + buffer[j];
					break;
				case PINDF_PNG_FLTR_TYPE_AVERAGE:
					cur_byte = *p + ((buffer[j - 1] + buffer[j]) / 2);
					break;
				default:
					PINDF_ERR("unsupported PNG filter type %d", filter_type);
					pindf_uchar_str_destroy(&temp_dest);
					return PINDF_FLTR_DAT_ERR;
				}

				buffer[j] = cur_byte;
			}
			memcpy(q, buffer + 1, row_size);
			q += row_size;
		}

		dest->len = n_rows * row_size;
		pindf_uchar_str_destroy(&temp_dest);
		return dest->len;
	}

	PINDF_ERR("predictor %d not implemented in flate decode!", f->decode_params.predictor);
	return PINDF_FLTR_DAT_ERR;
}

int pindf_flate_encode(pindf_stream_filter *f, pindf_uchar_str *dest, pindf_uchar_str *src)
{
	if (f->type != PINDF_FLTR_TYPE_FLATEDECODE) {
		PINDF_ERR("filter type %d not supported in flate encode!", f->type);
		return PINDF_FLTR_DAT_ERR;
	}

	if (f->decode_params.predictor == PINDF_FLTR_FLATE_PREDICTOR_NONE) {
		return pindf_zlib_compress(dest, src);
	}

	PINDF_ERR("predictor %d not implemented in flate encode!", f->decode_params.predictor);
	return PINDF_FLTR_DAT_ERR;
}

enum pindf_filter_type pindf_filter_type_from_name(const pindf_uchar_str *name)
{
	if (pindf_uchar_str_cmp3(name, "/FlateDecode") == 0) {
		return PINDF_FLTR_TYPE_FLATEDECODE;
	} else if (pindf_uchar_str_cmp3(name, "/ASCIIHexDecode") == 0) {
		return PINDF_FLTR_TYPE_ASCIIHEXDECODE;
	} else if (pindf_uchar_str_cmp3(name, "/ASCII85Decode") == 0) {
		return PINDF_FLTR_TYPE_ASCII85DECODE;
	} else if (pindf_uchar_str_cmp3(name, "/LZWDecode") == 0) {
		return PINDF_FLTR_TYPE_LZWDECODE;
	} else if (pindf_uchar_str_cmp3(name, "/RunLengthDecode") == 0) {
		return PINDF_FLTR_TYPE_RUNLENGTHDECODE;
	} else if (pindf_uchar_str_cmp3(name, "/CCITTFaxDecode") == 0) {
		return PINDF_FLTR_TYPE_CCITTFAXDECODE;
	} else if (pindf_uchar_str_cmp3(name, "/DCTDecode") == 0) {
		return PINDF_FLTR_TYPE_DCTDECODE;
	}

	return PINDF_FLTR_TYPE_NONE;
}

enum pindf_filter_type pindf_filter_type_from_name2(const char *name)
{
	if (strcmp(name, "/FlateDecode") == 0) {
		return PINDF_FLTR_TYPE_FLATEDECODE;
	} else if (strcmp(name, "/ASCIIHexDecode") == 0) {
		return PINDF_FLTR_TYPE_ASCIIHEXDECODE;
	} else if (strcmp(name, "/ASCII85Decode") == 0) {
		return PINDF_FLTR_TYPE_ASCII85DECODE;
	} else if (strcmp(name, "/LZWDecode") == 0) {
		return PINDF_FLTR_TYPE_LZWDECODE;
	} else if (strcmp(name, "/RunLengthDecode") == 0) {
		return PINDF_FLTR_TYPE_RUNLENGTHDECODE;
	} else if (strcmp(name, "/CCITTFaxDecode") == 0) {
		return PINDF_FLTR_TYPE_CCITTFAXDECODE;
	} else if (strcmp(name, "/DCTDecode") == 0) {
		return PINDF_FLTR_TYPE_DCTDECODE;
	}

	return PINDF_FLTR_TYPE_NONE;
}