#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "../type.h"
#include "../container/uchar_str.h"
#include "../container/simple_vector.h"

enum pindf_pdf_obj_type {
	PINDF_PDF_UNK = 0,
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
	/// vector of name obj
	pindf_vector *keys;
	/// vector of any obj (except for stream or ind)
	pindf_vector *values;
} pindf_pdf_dict;

typedef struct {
	uint64 content_offset;
	pindf_pdf_obj *dict;
	pindf_uchar_str *stream_content; /// original content, maybe compressed
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

/// @brief unified struct for all pdf objects
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

/// @brief new an object
/// @return created object. NULL if failed
pindf_pdf_obj *pindf_pdf_obj_new(enum pindf_pdf_obj_type type);

/// @brief get value by the key (key is a byte string)
pindf_pdf_obj *pindf_dict_getvalue(pindf_pdf_dict *dict, const uchar *key, size_t key_len);
/// @brief get value by the key (key is a c string)
pindf_pdf_obj *pindf_dict_getvalue2(pindf_pdf_dict *dict, const char *key);

/// @brief set value by the key (key is a byte string)
/// if key exists, replace the old value; otherwise, add new key-value pair
void pindf_dict_set_value(pindf_pdf_dict *dict, const uchar *key, size_t key_len, pindf_pdf_obj *value);

/// @brief set value by the key (key is a c string)
/// if key exists, replace the old value; otherwise, add new key-value pair
void pindf_dict_set_value2(pindf_pdf_dict *dict, const char *key, pindf_pdf_obj *value);

void pindf_pdf_obj_destroy(pindf_pdf_obj *obj);

/// @brief deep copy a pdf object
/// @param obj source object
/// @return newly allocated deep copy of the object, NULL if obj is NULL
pindf_pdf_obj *pindf_pdf_obj_deepcopy(const pindf_pdf_obj *obj);

/// @brief deep copy a pdf dict
/// @param dict source dictionary
/// @param dst destination dictionary (already allocated, will be initialized)
void pindf_dict_deepcopy(const pindf_pdf_dict *dict, pindf_pdf_dict *dst);

/// @brief deep copy a pdf array
/// @param arr source array (pindf_vector of pindf_pdf_obj*)
/// @param dst destination vector (already allocated, will be initialized)
void pindf_arr_deepcopy(const pindf_vector *arr, pindf_vector *dst);

/// @brief deep copy an indirect object
/// @param ind_obj source indirect object
/// @param dst destination indirect object (already allocated)
void pindf_ind_obj_deepcopy(const pindf_pdf_ind_obj *ind_obj, pindf_pdf_ind_obj *dst);

void pindf_pdf_dict_init(pindf_pdf_dict *dict);
void pindf_pdf_dict_destory(pindf_pdf_dict *dict);
void pindf_pdf_stream_destroy(pindf_pdf_stream *stream);
void pindf_pdf_ind_obj_destroy(pindf_pdf_ind_obj *ind_obj);

pindf_pdf_obj *pindf_pdf_name_from_cstr(const char *cstr);
pindf_pdf_obj *pindf_pdf_int_from_int(int num);