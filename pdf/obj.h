#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "../type.h"
#include "../container/uchar_str.h"
#include "../container/simple_vector.h"

#define PINDF_PDF_OBJ	0
#define PINDF_PDF_INT	1
#define PINDF_PDF_REAL	2
#define PINDF_PDF_DICT	3
#define PINDF_PDF_ARRAY	4
#define PINDF_PDF_LTR_STR	5
#define PINDF_PDF_HEX_STR	6
#define PINDF_PDF_BOOL	7
#define PINDF_PDF_STREAM	8
#define PINDF_PDF_NAME	9
#define PINDF_PDF_NULL	10
#define PINDF_PDF_REF	11
#define PINDF_PDF_IND_OBJ	12

typedef struct pindf_pdf_obj pindf_pdf_obj;

typedef struct {
	pindf_vector *keys; // vector of name obj
	pindf_vector *values; // vector of obj
} pindf_pdf_dict;

typedef struct  {
	pindf_vector *items;
} pindf_pdf_array;

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
	int obj_type;
	union {
		int boolean;
		int num;
		double real_num;
		pindf_pdf_ref ref;

		pindf_uchar_str *name;
		pindf_uchar_str *hex_str;
		pindf_uchar_str *ltr_str;

		pindf_pdf_dict dict;
		pindf_pdf_array array;

		pindf_pdf_stream stream;

		pindf_pdf_ind_obj indirect_obj;
	} content;
};

pindf_pdf_obj *pindf_pdf_obj_new(int type);