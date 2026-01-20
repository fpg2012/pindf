#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "type.h"
#include "uchar_str.h"
#include "simple_vector.h"

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

struct pindf_pdf_obj;
struct pindf_pdf_dict;
struct pindf_pdf_array;
struct pindf_stream;
struct pindf_pdf_ind_obj;

struct pindf_pdf_dict {
	struct pindf_vector *keys; // vector of name obj
	struct pindf_vector *values; // vector of obj
};

struct pindf_pdf_array {
	struct pindf_vector *items;
};

struct pindf_pdf_stream {
	struct pindf_pdf_obj *dict;
	struct pindf_uchar_str *stream_content;
};

struct pindf_pdf_ind_obj {
	int obj_num;
	int generation_num;
	struct pindf_pdf_obj *obj;
	size_t start_pos;
};

struct pindf_pdf_ref {
	int obj_num;
	int generation_num;
};

struct pindf_pdf_obj {
	int obj_type;
	union {
		int boolean;
		int num;
		double real_num;
		struct pindf_pdf_ref ref;

		struct pindf_uchar_str *name;
		struct pindf_uchar_str *hex_str;
		struct pindf_uchar_str *ltr_str;

		struct pindf_pdf_dict dict;
		struct pindf_pdf_array array;

		struct pindf_pdf_stream stream;

		struct pindf_pdf_ind_obj indirect_obj;
	} content;
};

struct pindf_pdf_obj *pindf_pdf_obj_new(int type);
char *pindf_pdf_obj_serialize(struct pindf_pdf_obj *obj, char *buf);
char *pindf_pdf_obj_serialize_json(struct pindf_pdf_obj *obj, char *buf);