#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pdf/obj.h"
#include "../pdf/doc.h"
#include "../container/simple_vector.h"

/// @brief serialize pdf_obj back to pdf format
char *pindf_pdf_obj_serialize(pindf_pdf_obj *obj, char *buf, size_t buf_size);

/// @brief serialize pdf_obj to json object
/// note that content of stream object will not included in the json object
char *pindf_pdf_obj_serialize_json(pindf_pdf_obj *obj, char *buf, size_t buf_size);