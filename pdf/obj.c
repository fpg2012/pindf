#include "obj.h"

struct pindf_pdf_obj *pindf_pdf_obj_new(int obj_type)
{
	struct pindf_pdf_obj *obj = (struct pindf_pdf_obj*)malloc(sizeof(struct pindf_pdf_obj));
	obj->obj_type = obj_type;
	return obj;
}