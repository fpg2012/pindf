#include "serialize.h"
#include "../logger/logger.h"

#define BUF_REMAIN (buf_size-(p-buf))

char *pindf_escape_copy(char *dest, char *src, size_t len, size_t buf_size, char *buf)
{
	char *q = src, *end = src + len;
	char *p = dest;
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
	return p;
}

char *pindf_dict_serialize_json(pindf_pdf_dict *dict, char *buf, size_t buf_size)
{
	char *p = buf;
	pindf_pdf_obj *temp_obj;

	p += snprintf(p, BUF_REMAIN, "{");
	pindf_vector *keys = dict->keys;
	pindf_vector *values = dict->values;
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
	return p;
}

char *pindf_pdf_obj_serialize_json(pindf_pdf_obj *obj, char *buf, size_t buf_size)
{
	char *p = buf;
	pindf_pdf_obj *temp_obj;

	switch (obj->obj_type) {
	case PINDF_PDF_DICT: {
		p = pindf_dict_serialize_json(&obj->content.dict, buf, buf_size);
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
		p = pindf_escape_copy(p, (char*)obj->content.ltr_str->p, obj->content.ltr_str->len, buf_size, buf);
		p += snprintf(p, BUF_REMAIN, "\"");
		break;
	}
	case PINDF_PDF_HEX_STR:
		p += snprintf(p, BUF_REMAIN, "{\"type\": \"hex\",\"str\":\"");
		for (int i = 0; i < obj->content.hex_str->len; i++) {
			p += snprintf(p, BUF_REMAIN, "%02x", obj->content.hex_str->p[i]);
		}
		p += snprintf(p, BUF_REMAIN, "\"}");
		break;
	case PINDF_PDF_NAME:
		p += snprintf(p, BUF_REMAIN, "\"");
		p = pindf_escape_copy(p, (char*)obj->content.name->p, obj->content.name->len, buf_size, buf);
		p += snprintf(p, BUF_REMAIN, "\"");
		break;
	case PINDF_PDF_STREAM:
		p += snprintf(p, BUF_REMAIN, "{\"type\":\"stream\",\"content_offset\":%llu, \"dict\":", obj->content.stream.content_offset);
		p = pindf_dict_serialize_json(&obj->content.stream.dict->content.dict, p, BUF_REMAIN);
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

char *pindf_dict_serialize(pindf_pdf_dict *dict, char *buf, size_t buf_size)
{
	char *p = buf;
	pindf_pdf_obj *temp_obj;

	p += snprintf(p, BUF_REMAIN, "<< ");
	pindf_vector *keys = dict->keys;
	pindf_vector *values = dict->values;
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
	return p;
}

char *pindf_ind_obj_serialize(pindf_pdf_ind_obj *ind_obj, char *buf, size_t buf_size)
{
	char *p = buf;

	p += snprintf(p, BUF_REMAIN, "%d %d obj\r\n", ind_obj->obj_num, ind_obj->generation_num);
	p = pindf_pdf_obj_serialize(ind_obj->obj, p, BUF_REMAIN);
	p += snprintf(p, BUF_REMAIN, "\r\nendobj\r\n");
	return p;
}

char *pindf_pdf_obj_serialize(pindf_pdf_obj *obj, char *buf, size_t buf_size)
{
	char *p = buf;
	pindf_pdf_obj *temp_obj;

	switch (obj->obj_type) {
	case PINDF_PDF_DICT: {
		p = pindf_dict_serialize(&obj->content.dict, buf, buf_size);
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

		p += snprintf(p, BUF_REMAIN, "]");
		break;
	}
	case PINDF_PDF_REF:
		p += snprintf(p, BUF_REMAIN, "%d %d R", obj->content.ref.obj_num, obj->content.ref.generation_num);
		break;
	case PINDF_PDF_IND_OBJ:
		p = pindf_ind_obj_serialize(&obj->content.indirect_obj, buf, buf_size);
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
		break;
	default:
		PINDF_ERR("Invalid object type");
	}

	return p;
}


// to file

void pindf_dict_serialize_file(pindf_pdf_dict *dict, FILE *fp)
{
	pindf_pdf_obj *temp_key, *temp_value;

	fprintf(fp, "<< ");
	pindf_vector *keys = dict->keys;
	pindf_vector *values = dict->values;
	size_t len = keys->len;
	
	for (int i = 0; i < len; ++i) {
		pindf_vector_index(keys, i, &temp_key);
		pindf_vector_index(values, i, &temp_value);
		
		if (temp_value != NULL) {
			pindf_pdf_obj_serialize_file(temp_key, fp);
			fprintf(fp, " ");
			pindf_pdf_obj_serialize_file(temp_value, fp);
			fprintf(fp, " ");
		}
	}

	fprintf(fp, ">>");
}

void pindf_ind_obj_serialize_file(pindf_pdf_ind_obj *ind_obj, FILE *fp)
{
	fprintf(fp, "%d %d obj\r\n", ind_obj->obj_num, ind_obj->generation_num);
	pindf_pdf_obj_serialize_file(ind_obj->obj, fp);
	fprintf(fp, "\r\nendobj\r\n");
}

void pindf_pdf_obj_serialize_file(pindf_pdf_obj *obj, FILE *fp)
{
	pindf_pdf_obj *temp_obj;

	switch (obj->obj_type) {
	case PINDF_PDF_DICT: {
		pindf_dict_serialize_file(&obj->content.dict, fp);
		break;
	}
	case PINDF_PDF_ARRAY: {
		fprintf(fp, "[ ");

		pindf_vector *vec = obj->content.array;
		size_t len = vec->len;

		for (int i = 0; i < len; ++i) {
			pindf_vector_index(vec, i, &temp_obj);
			pindf_pdf_obj_serialize_file(temp_obj, fp);
			fprintf(fp, " ");
		}

		fprintf(fp, "]");
		break;
	}
	case PINDF_PDF_REF:
		fprintf(fp, "%d %d R", obj->content.ref.obj_num, obj->content.ref.generation_num);
		break;
	case PINDF_PDF_IND_OBJ:
		pindf_ind_obj_serialize_file(&obj->content.indirect_obj, fp);
		break;
	case PINDF_PDF_INT:
		fprintf(fp, "%d", obj->content.num);
		break;
	case PINDF_PDF_REAL:
		fprintf(fp, "%lf", obj->content.real_num);
		break;
	case PINDF_PDF_LTR_STR: {
		fprintf(fp, "(");
		fwrite(obj->content.ltr_str->p, 1, obj->content.ltr_str->len, fp);
		fprintf(fp, ")");
		break;
	}
	case PINDF_PDF_HEX_STR:
		fprintf(fp, "<");
		for (int i = 0; i < obj->content.hex_str->len; i++) {
			fprintf(fp, "%02x", obj->content.hex_str->p[i]);
		}
		fprintf(fp, ">");
		break;
	case PINDF_PDF_NAME:
		fprintf(fp, "%s", (char*)obj->content.name->p);
		break;
	case PINDF_PDF_STREAM:
		pindf_pdf_obj_serialize_file(obj->content.stream.dict, fp);
		fprintf(fp, "\r\nstream\r\n");

		fwrite(obj->content.stream.stream_content->p, 1, obj->content.stream.stream_content->len, fp);
		
		fprintf(fp, "\r\nendstream");
		break;
	case PINDF_PDF_NULL:
		fprintf(fp, "null");
		break;
	case PINDF_PDF_BOOL:
		fprintf(fp, "%s", obj->content.boolean ? "true" : "false");
		break;
	default:
		PINDF_ERR("Invalid object type");
	}
}