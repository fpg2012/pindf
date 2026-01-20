#include "pdf_obj.h"
#include "simple_vector.h"
#include "uchar_str.h"
#include <stdio.h>

struct pindf_pdf_obj *pindf_pdf_obj_new(int obj_type)
{
	struct pindf_pdf_obj *obj = (struct pindf_pdf_obj*)malloc(sizeof(struct pindf_pdf_obj));
	obj->obj_type = obj_type;
	return obj;
}

char *pindf_pdf_obj_serialize_json(struct pindf_pdf_obj *obj, char *buf)
{
	struct pindf_uchar_str *str = NULL;
	char *p = buf;
	struct pindf_pdf_obj *temp_obj;

	switch (obj->obj_type) {
	case PINDF_PDF_DICT: {
		p += sprintf(p, "{");

		
		struct pindf_vector *keys = obj->content.dict.keys;
		struct pindf_vector *values = obj->content.dict.values;
		size_t len = keys->len;
		
		for (int i = 0; i < len; ++i) {
			pindf_vector_index(keys, i, &temp_obj);
			p = pindf_pdf_obj_serialize_json(temp_obj, p);

			p += sprintf(p, ":");

			pindf_vector_index(values, i, &temp_obj);
			p = pindf_pdf_obj_serialize_json(temp_obj, p);
			if (i < len - 1)
				p += sprintf(p, ",");
		}

		p += sprintf(p, "}");
		break;
	}
	case PINDF_PDF_ARRAY: {
		p += sprintf(p, "[");

		struct pindf_vector *vec = obj->content.array.items;
		size_t len = vec->len;

		for (int i = 0; i < len; ++i) {
			pindf_vector_index(vec, i, &temp_obj);
			p = pindf_pdf_obj_serialize_json(temp_obj, p);
			if (i != len - 1)
				p += sprintf(p, ",");
		}

		p += sprintf(p, "]");
		break;
	}
	case PINDF_PDF_REF:
		p += sprintf(p, "{\"type\": \"ref\",\"a\":%d,\"b\":%d}", obj->content.ref.obj_num, obj->content.ref.generation_num);
		break;
	case PINDF_PDF_IND_OBJ:
		p += sprintf(p, "{\"type\": \"ind\",\"a\":%d,\"b\":%d,\"obj\":", obj->content.indirect_obj.obj_num, obj->content.indirect_obj.generation_num);
		p = pindf_pdf_obj_serialize_json(obj->content.indirect_obj.obj, p);
		p += sprintf(p, "}");
		break;
	case PINDF_PDF_INT:
		p += sprintf(p, "%d", obj->content.num);
		break;
	case PINDF_PDF_REAL:
		p += sprintf(p, "%lf", obj->content.real_num);
		break;
	case PINDF_PDF_LTR_STR: {
		p += sprintf(p, "\"");
		char *q, *end;
		q = (char*)obj->content.ltr_str->p;
		end = q + obj->content.ltr_str->len;
		while (q != end) {
			if (*q == '\n') {
				p += sprintf(p, "\\n");
			} else if (*q == '\r') {
				p += sprintf(p, "\\r");	
			} else if (*q == '\0') {
				p += sprintf(p, "\\0");
			} else if (*q == '\"') {
				p += sprintf(p, "\\\"");
			} else if (*q == '\\') {
				p += sprintf(p, "\\\\");
			} else if (*q < 0) {
				p += sprintf(p, "\\x%02x", (uint)*q);
			} else {
				*(p++) = *q;
			}
			q++;
		}
		p += sprintf(p, "\"");
		break;
	}
	case PINDF_PDF_HEX_STR:
		p += sprintf(p, "{\"type\": \"hex\",\"str\":\"%s\"}", (char*)obj->content.hex_str->p);
		break;
	case PINDF_PDF_NAME:
	p += sprintf(p, "\"");
		char *q, *end;
		q = (char*)obj->content.name->p;
		end = q + obj->content.name->len;
		while (q != end) {
			if (*q == '\n') {
				p += sprintf(p, "\\n");
			} else if (*q == '\r') {
				p += sprintf(p, "\\r");	
			} else if (*q == '\0') {
				p += sprintf(p, "\\0");
			} else if (*q == '\"') {
				p += sprintf(p, "\\\"");
			} else if (*q == '\\') {
				p += sprintf(p, "\\\\");
			} else if (*q < 0) {
				p += sprintf(p, "\\%0o", (uint)*q);
			} else {
				*(p++) = *q;
			}
			q++;
		}
		p += sprintf(p, "\"");
		break;
	case PINDF_PDF_STREAM:
		p += sprintf(p, "{\"type\": \"stream\", \"dict\":");
		p = pindf_pdf_obj_serialize_json(obj->content.stream.dict, p);
		p += sprintf(p, ",\"len\": \"%lu\"", obj->content.stream.stream_content->len);
		p += sprintf(p, "}");
	}
	return p;
}

char *pindf_pdf_obj_serialize(struct pindf_pdf_obj *obj, char *buf)
{
	struct pindf_uchar_str *str = NULL;
	char *p = buf;
	struct pindf_pdf_obj *temp_obj;

	switch (obj->obj_type) {
	case PINDF_PDF_DICT: {
		p += sprintf(p, "<< ");

		
		struct pindf_vector *keys = obj->content.dict.keys;
		struct pindf_vector *values = obj->content.dict.values;
		size_t len = keys->len;
		
		for (int i = 0; i < len; ++i) {
			pindf_vector_index(keys, i, &temp_obj);
			p = pindf_pdf_obj_serialize(temp_obj, p);
			p += sprintf(p, " ");

			pindf_vector_index(values, i, &temp_obj);
			p = pindf_pdf_obj_serialize(temp_obj, p);
			p += sprintf(p, " ");
		}

		p += sprintf(p, ">>");
		break;
	}
	case PINDF_PDF_ARRAY: {
		p += sprintf(p, "[ ");

		struct pindf_vector *vec = obj->content.array.items;
		size_t len = vec->len;

		for (int i = 0; i < len; ++i) {
			pindf_vector_index(vec, i, &temp_obj);
			p = pindf_pdf_obj_serialize(temp_obj, p);
			p += sprintf(p, " ");
		}

		sprintf(p, "]"); p += 1;
		break;
	}
	case PINDF_PDF_REF:
		p += sprintf(p, "%d %d R", obj->content.ref.obj_num, obj->content.ref.generation_num);
		break;
	case PINDF_PDF_IND_OBJ:
		p += sprintf(p, "%d %d obj\r\n", obj->content.indirect_obj.obj_num, obj->content.indirect_obj.generation_num);
		p = pindf_pdf_obj_serialize(obj->content.indirect_obj.obj, p);
		p += sprintf(p, "\r\nendobj\r\n");
		break;
	case PINDF_PDF_INT:
		p += sprintf(p, "%d", obj->content.num);
		break;
	case PINDF_PDF_REAL:
		p += sprintf(p, "%lf", obj->content.real_num);
		break;
	case PINDF_PDF_LTR_STR: {
		p += sprintf(p, "(");
		char *q, *end;
		q = (char*)obj->content.ltr_str->p;
		end = q + obj->content.ltr_str->len;
		while (q != end) {
			*(p++) = *(q++);
		}
		p += sprintf(p, ")");
		break;
	}
	case PINDF_PDF_HEX_STR:
		p += sprintf(p, "<%s>", (char*)obj->content.hex_str->p);
		break;
	case PINDF_PDF_NAME:
		p += sprintf(p, "%s", (char*)obj->content.name->p);
		break;
	case PINDF_PDF_STREAM:
		p = pindf_pdf_obj_serialize(obj->content.stream.dict, p);
		p += sprintf(p, "\r\nstream\r\n");

		memcpy(p, obj->content.stream.stream_content->p, obj->content.stream.stream_content->len);
		p += obj->content.stream.stream_content->len;
		
		p += sprintf(p, "\r\nendstream");
	}

	return p;
}