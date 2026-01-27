#pragma once

#include <zlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../container/uchar_str.h"
#include "../pdf/obj.h"

#define PINDF_MAX_EXPAND_RETRY 4

enum pindf_filter_errno {
	PINDF_FLTR_OK = 0,
	PINDF_FLTR_MEM_ERR = -1,
	PINDF_FLTR_DAT_ERR = -2,
};

enum pindf_filter_type {
	PINDF_FLTR_TYPE_NONE = 0,
	PINDF_FLTR_TYPE_FLATEDECODE,
	PINDF_FLTR_TYPE_ASCIIHEXDECODE,
	PINDF_FLTR_TYPE_ASCII85DECODE,
	PINDF_FLTR_TYPE_LZWDECODE,
	PINDF_FLTR_TYPE_RUNLENGTHDECODE,
	PINDF_FLTR_TYPE_CCITTFAXDECODE,
	PINDF_FLTR_TYPE_DCTDECODE,
};

enum pindf_filter_flate_predictor {
	PINDF_FLTR_FLATE_PREDICTOR_UNKNOWN = 0,
	PINDF_FLTR_FLATE_PREDICTOR_NONE = 1,
	PINDF_FLTR_FLATE_PREDICTOR_TIFF_2 = 2,
	PINDF_FLTR_FLATE_PREDICTOR_PNG_UP = 11,
	PINDF_FLTR_FLATE_PREDICTOR_PNG_SUB = 12,
	PINDF_FLTR_FLATE_PREDICTOR_PNG_AVERAGE = 13,
	PINDF_FLTR_FLATE_PREDICTOR_PNG_PAETH = 14,
	PINDF_FLTR_FLATE_PREDTCTOR_PNG_OPTIMUM = 15,
};

enum pindf_png_filter_type {
	PINDF_PNG_FLTR_TYPE_NONE = 0,
	PINDF_PNG_FLTR_TYPE_SUB = 1,
	PINDF_PNG_FLTR_TYPE_UP = 2,
	PINDF_PNG_FLTR_TYPE_AVERAGE = 3,
	PINDF_PNG_FLTR_TYPE_PAETH = 4,
};

typedef struct {
	enum pindf_filter_flate_predictor predictor;
	int colors;
	int bits_per_component;
	int columns;
} pindf_filter_decode_params;

typedef struct pindf_stream_filter pindf_stream_filter;

typedef struct pindf_stream_filter {
	enum pindf_filter_type type;

	pindf_filter_decode_params decode_params;

	enum pindf_filter_errno (*decode)(pindf_stream_filter *f, pindf_uchar_str *dest, pindf_uchar_str *src);
	enum pindf_filter_errno (*encode)(pindf_stream_filter *f, pindf_uchar_str *dest, pindf_uchar_str *src);
} pindf_stream_filter;


int pindf_filter_init(pindf_stream_filter *filter, enum pindf_filter_type type, pindf_pdf_dict *decode_params);
void pindf_filter_destroy(pindf_stream_filter *filter);

int pindf_zlib_uncompress(pindf_uchar_str *dest, pindf_uchar_str *src);
int pindf_zlib_compress(pindf_uchar_str *dest, pindf_uchar_str *src);

int pindf_flate_decode(pindf_stream_filter *f, pindf_uchar_str *dest, pindf_uchar_str *src);
int pindf_flate_encode(pindf_stream_filter *f, pindf_uchar_str *dest, pindf_uchar_str *src);