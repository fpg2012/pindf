#include "serialize.h"
#include "../logger/logger.h"

#define BUF_REMAIN (buf_size-(p-buf))

char *pindf_pdf_obj_serialize_json(pindf_pdf_obj *obj, char *buf, size_t buf_size)
{
	pindf_uchar_str *str = NULL;
	char *p = buf;
	pindf_pdf_obj *temp_obj;

	switch (obj->obj_type) {
	case PINDF_PDF_DICT: {
		p += snprintf(p, BUF_REMAIN, "{");
		pindf_vector *keys = obj->content.dict.keys;
		pindf_vector *values = obj->content.dict.values;
		size_t len = keys->len;
		
		for (int i = 0; i < len; ++i) {
			pindf_vector_index(keys, i, &temp_obj);
			p = pindf_pdf_obj_serialize_json(temp_obj, p, BUF_REMAIN);

			p += snprintf(p, BUF_REMAIN, ":");

			pindf_vector_index(values, i, &temp_obj);
			p = pindf_pdf_obj_serialize_json(temp_obj, p, BUF_REMAIN);
			if (i < len - 1)
				p += snprintf(p, BUF_REMAIN, ",");
		}

		p += snprintf(p, BUF_REMAIN, "}");
		break;
	}
	case PINDF_PDF_ARRAY: {
		p += snprintf(p, BUF_REMAIN, "[");

		pindf_vector *vec = obj->content.array;
		size_t len = vec->len;

		for (int i = 0; i < len; ++i) {
			pindf_vector_index(vec, i, &temp_obj);
			p = pindf_pdf_obj_serialize_json(temp_obj, p, BUF_REMAIN);
			if (i != len - 1)
				p += snprintf(p, BUF_REMAIN, ",");
		}

		p += snprintf(p, BUF_REMAIN, "]");
		break;
	}
	case PINDF_PDF_REF:
		p += snprintf(p, BUF_REMAIN, "{\"type\": \"ref\",\"a\":%d,\"b\":%d}", obj->content.ref.obj_num, obj->content.ref.generation_num);
		break;
	case PINDF_PDF_IND_OBJ:
		p += snprintf(p, BUF_REMAIN, "{\"type\": \"ind\",\"a\":%d,\"b\":%d,\"obj\":", obj->content.indirect_obj.obj_num, obj->content.indirect_obj.generation_num);
		p = pindf_pdf_obj_serialize_json(obj->content.indirect_obj.obj, p, BUF_REMAIN);
		p += snprintf(p, BUF_REMAIN, "}");
		break;
	case PINDF_PDF_INT:
		p += snprintf(p, BUF_REMAIN, "%d", obj->content.num);
		break;
	case PINDF_PDF_REAL:
		p += snprintf(p, BUF_REMAIN, "%lf", obj->content.real_num);
		break;
	case PINDF_PDF_LTR_STR: {
		p += snprintf(p, BUF_REMAIN, "\"");
		char *q, *end;
		q = (char*)obj->content.ltr_str->p;
		end = q + obj->content.ltr_str->len;
		while (q != end) {
			if (*q == '\n') {
				p += snprintf(p, BUF_REMAIN, "\\n");
			} else if (*q == '\r') {
				p += snprintf(p, BUF_REMAIN, "\\r");	
			} else if (*q == '\0') {
				p += snprintf(p, BUF_REMAIN, "\\0");
			} else if (*q == '\"') {
				p += snprintf(p, BUF_REMAIN, "\\\"");
			} else if (*q == '\\') {
				p += snprintf(p, BUF_REMAIN, "\\\\");
			} else if (*q < 0) {
				p += snprintf(p, BUF_REMAIN, "\\x%02x", 0xFF & (uint)*q);
			} else {
				*(p++) = *q;
			}
			q++;
		}
		p += snprintf(p, BUF_REMAIN, "\"");
		break;
	}
	case PINDF_PDF_HEX_STR:
		p += snprintf(p, BUF_REMAIN, "{\"type\": \"hex\",\"str\":\"%s\"}", (char*)obj->content.hex_str->p);
		break;
	case PINDF_PDF_NAME:
	p += snprintf(p, BUF_REMAIN, "\"");
		char *q, *end;
		q = (char*)obj->content.name->p;
		end = q + obj->content.name->len;
		while (q != end) {
			if (*q == '\n') {
				p += snprintf(p, BUF_REMAIN, "\\n");
			} else if (*q == '\r') {
				p += snprintf(p, BUF_REMAIN, "\\r");	
			} else if (*q == '\0') {
				p += snprintf(p, BUF_REMAIN, "\\0");
			} else if (*q == '\"') {
				p += snprintf(p, BUF_REMAIN, "\\\"");
			} else if (*q == '\\') {
				p += snprintf(p, BUF_REMAIN, "\\\\");
			} else if (*q < 0) {
				p += snprintf(p, BUF_REMAIN, "\\%0o", (uint)*q);
			} else {
				*(p++) = *q;
			}
			q++;
		}
		p += snprintf(p, BUF_REMAIN, "\"");
		break;
	case PINDF_PDF_STREAM:
		p += snprintf(p, BUF_REMAIN, "{\"type\":\"stream\",\"content_offset\":%llu, \"dict\":", obj->content.stream.content_offset);
		p = pindf_pdf_obj_serialize_json(obj->content.stream.dict, p, BUF_REMAIN);
		p += snprintf(p, BUF_REMAIN, ",\"len\":\"%lu\"", obj->content.stream.stream_content->len);
		p += snprintf(p, BUF_REMAIN, "}");
		break;
	case PINDF_PDF_NULL:
		p += snprintf(p, BUF_REMAIN, "null");
		break;
	case PINDF_PDF_BOOL:
		p += snprintf(p, BUF_REMAIN, "%s", obj->content.boolean ? "true" : "false");
		break;
	default:
		PINDF_ERR("invalid obj type");
	}
	return p;
}

char *pindf_pdf_obj_serialize(pindf_pdf_obj *obj, char *buf, size_t buf_size)
{
	pindf_uchar_str *str = NULL;
	char *p = buf;
	pindf_pdf_obj *temp_obj;

	switch (obj->obj_type) {
	case PINDF_PDF_DICT: {
		p += snprintf(p, BUF_REMAIN, "<< ");

		
		pindf_vector *keys = obj->content.dict.keys;
		pindf_vector *values = obj->content.dict.values;
		size_t len = keys->len;
		
		for (int i = 0; i < len; ++i) {
			pindf_vector_index(keys, i, &temp_obj);
			p = pindf_pdf_obj_serialize(temp_obj, p, BUF_REMAIN);
			p += snprintf(p, BUF_REMAIN, " ");

			pindf_vector_index(values, i, &temp_obj);
			p = pindf_pdf_obj_serialize(temp_obj, p, BUF_REMAIN);
			p += snprintf(p, BUF_REMAIN, " ");
		}

		p += snprintf(p, BUF_REMAIN, ">>");
		break;
	}
	case PINDF_PDF_ARRAY: {
		p += snprintf(p, BUF_REMAIN, "[ ");

		pindf_vector *vec = obj->content.array;
		size_t len = vec->len;

		for (int i = 0; i < len; ++i) {
			pindf_vector_index(vec, i, &temp_obj);
			p = pindf_pdf_obj_serialize(temp_obj, p, BUF_REMAIN);
			p += snprintf(p, BUF_REMAIN, " ");
		}

		snprintf(p, BUF_REMAIN, "]"); p += 1;
		break;
	}
	case PINDF_PDF_REF:
		p += snprintf(p, BUF_REMAIN, "%d %d R", obj->content.ref.obj_num, obj->content.ref.generation_num);
		break;
	case PINDF_PDF_IND_OBJ:
		p += snprintf(p, BUF_REMAIN, "%d %d obj\r\n", obj->content.indirect_obj.obj_num, obj->content.indirect_obj.generation_num);
		p = pindf_pdf_obj_serialize(obj->content.indirect_obj.obj, p, BUF_REMAIN);
		p += snprintf(p, BUF_REMAIN, "\r\nendobj\r\n");
		break;
	case PINDF_PDF_INT:
		p += snprintf(p, BUF_REMAIN, "%d", obj->content.num);
		break;
	case PINDF_PDF_REAL:
		p += snprintf(p, BUF_REMAIN, "%lf", obj->content.real_num);
		break;
	case PINDF_PDF_LTR_STR: {
		p += snprintf(p, BUF_REMAIN, "(");
		char *q, *end;
		q = (char*)obj->content.ltr_str->p;
		end = q + obj->content.ltr_str->len;
		while (q != end) {
			*(p++) = *(q++);
		}
		p += snprintf(p, BUF_REMAIN, ")");
		break;
	}
	case PINDF_PDF_HEX_STR:
		p += snprintf(p, BUF_REMAIN, "<%s>", (char*)obj->content.hex_str->p);
		break;
	case PINDF_PDF_NAME:
		p += snprintf(p, BUF_REMAIN, "%s", (char*)obj->content.name->p);
		break;
	case PINDF_PDF_STREAM:
		p = pindf_pdf_obj_serialize(obj->content.stream.dict, p, BUF_REMAIN);
		p += snprintf(p, BUF_REMAIN, "\r\nstream\r\n");

		memcpy(p, obj->content.stream.stream_content->p, obj->content.stream.stream_content->len);
		p += obj->content.stream.stream_content->len;
		
		p += snprintf(p, BUF_REMAIN, "\r\nendstream");
		break;
	case PINDF_PDF_NULL:
		p += snprintf(p, BUF_REMAIN, "null");
		break;
	case PINDF_PDF_BOOL:
		p += snprintf(p, BUF_REMAIN, "%s", obj->content.boolean ? "true" : "false");
	default:
		fprintf(stderr, "[error] invalid obj type");
	}

	return p;
}