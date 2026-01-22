#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pdf/obj.h"
#include "../pdf/doc.h"
#include "../container/simple_vector.h"

char *pindf_doc_serialize_json(pindf_doc *doc, char *buf, size_t buf_size);
char *pindf_pdf_obj_serialize(pindf_pdf_obj *obj, char *buf, size_t buf_size);
char *pindf_pdf_obj_serialize_json(pindf_pdf_obj *obj, char *buf, size_t buf_size);