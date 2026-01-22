#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "../type.h"
#include "../container/uchar_str.h"
#include "../container/simple_vector.h"

enum pindf_pdf_obj_type {
	PINDF_PDF_OBJ,
	PINDF_PDF_INT,
	PINDF_PDF_REAL,
	PINDF_PDF_DICT,
	PINDF_PDF_ARRAY,
	PINDF_PDF_LTR_STR,
	PINDF_PDF_HEX_STR,
	PINDF_PDF_BOOL,
	PINDF_PDF_STREAM,
	PINDF_PDF_NAME,	
	PINDF_PDF_NULL,	
	PINDF_PDF_REF,
	PINDF_PDF_IND_OBJ,
};

typedef struct pindf_pdf_obj pindf_pdf_obj;

typedef struct {
	pindf_vector *keys; // vector of name obj
	pindf_vector *values; // vector of obj
} pindf_pdf_dict;

typedef struct {
	pindf_pdf_obj *dict;
	pindf_uchar_str *stream_content;
} pindf_pdf_stream;

typedef struct {
	int obj_num;
	int generation_num;
	pindf_pdf_obj *obj;
	size_t start_pos;
} pindf_pdf_ind_obj;

typedef struct {
	int obj_num;
	int generation_num;
} pindf_pdf_ref;

struct pindf_pdf_obj {
	enum pindf_pdf_obj_type obj_type;
	union {
		int boolean;
		int num;
		double real_num;
		pindf_pdf_ref ref;

		pindf_uchar_str *name;
		pindf_uchar_str *hex_str;
		pindf_uchar_str *ltr_str;
		pindf_vector *array;

		pindf_pdf_dict dict;

		pindf_pdf_stream stream;

		pindf_pdf_ind_obj indirect_obj;
	} content;
};

pindf_pdf_obj *pindf_pdf_obj_new(enum pindf_pdf_obj_type type);
pindf_pdf_obj *pindf_dict_getvalue(pindf_pdf_dict *dict, pindf_uchar_str *key);