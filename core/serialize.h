/*
 Copyright 2026 fpg2012 (aka nth233)

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

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