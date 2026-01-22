#include "obj.h"

pindf_pdf_obj *pindf_pdf_obj_new(int obj_type)
{
	pindf_pdf_obj *obj = (pindf_pdf_obj*)malloc(sizeof(pindf_pdf_obj));
	obj->obj_type = obj_type;
	return obj;
}