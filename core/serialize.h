#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pdf/obj.h"


/// @brief serialize pdf_obj back to pdf format
char *pindf_pdf_obj_serialize(pindf_pdf_obj *obj, char *buf, size_t buf_size);

char *pindf_dict_serialize(pindf_pdf_dict *dict, char *buf, size_t buf_size);
char *pindf_ind_obj_serialize(pindf_pdf_ind_obj *ind_obj, char *buf, size_t buf_size);

/* to file */

/// @brief serialize pdf_obj back to pdf format to file
void pindf_pdf_obj_serialize_file(pindf_pdf_obj *obj, FILE *fp);

void pindf_dict_serialize_file(pindf_pdf_dict *dict, FILE *fp);
void pindf_ind_obj_serialize_file(pindf_pdf_ind_obj *ind_obj, FILE *fp);

/* to json */

/// @brief serialize pdf_obj to json object
/// note that content of stream object will not included in the json object
char *pindf_pdf_obj_serialize_json(pindf_pdf_obj *obj, char *buf, size_t buf_size);

char *pindf_dict_serialize_json(pindf_pdf_dict *dict, char *buf, size_t buf_size);