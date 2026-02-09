#include "obj.h"
#include "../logger/logger.h"

pindf_pdf_obj *pindf_pdf_obj_new(enum pindf_pdf_obj_type obj_type)
{
	pindf_pdf_obj *obj = (pindf_pdf_obj*)malloc(sizeof(pindf_pdf_obj));
	obj->obj_type = obj_type;
	return obj;
}

pindf_pdf_obj *pindf_dict_getvalue(pindf_pdf_dict *dict, const uchar *key, size_t key_len)
{
	assert(key != NULL);

	pindf_pdf_obj **beg = (pindf_pdf_obj **)dict->keys->buf;
	pindf_pdf_obj **end = beg + dict->keys->len;

	pindf_pdf_obj *value = NULL;
	pindf_pdf_obj *cur_key = NULL;

	size_t index = 0;
	for (pindf_pdf_obj **p = beg; p != end; ++p, ++index) {
		cur_key = *p;
		if (cur_key->obj_type != PINDF_PDF_NAME) {
			PINDF_ERR("non-name dict key");
			return NULL;
		}
		if (pindf_uchar_str_cmp2(cur_key->content.name, key, key_len) == 0) {
			break;
		}
	}

	pindf_vector_index(dict->values, index, &value);
	return value;
}

pindf_pdf_obj *pindf_dict_getvalue2(pindf_pdf_dict *dict, const char *key)
{
	assert(key != NULL);
	size_t len =  strlen(key);

	pindf_pdf_obj **beg = (pindf_pdf_obj **)dict->keys->buf;
	pindf_pdf_obj **end = beg + dict->keys->len;

	pindf_pdf_obj *value = NULL;
	pindf_pdf_obj *cur_key = NULL;

	size_t index = 0;
	for (pindf_pdf_obj **p = beg; p != end; ++p, ++index) {
		cur_key = *p;
		if (cur_key->obj_type != PINDF_PDF_NAME) {
			PINDF_ERR("non-name dict key");
			return NULL;
		}
		if (pindf_uchar_str_cmp2(cur_key->content.name, (const uchar*)key, len) == 0) {
			break;
		}
	}

	pindf_vector_index(dict->values, index, &value);
	return value;
}

void pindf_pdf_dict_destory(pindf_pdf_dict *dict)
{
	if (dict->keys != NULL) {
		for (int i = 0; i < dict->keys->len; ++i) {
			pindf_pdf_obj *temp = NULL;
			pindf_vector_index(dict->keys, i, &temp);
			pindf_pdf_obj_destroy(temp);
			free(temp);
		}
		pindf_vector_destroy(dict->keys);
		free(dict->keys);
	}

	if (dict->values != NULL) {
		for (int i = 0; i < dict->values->len; ++i) {
			pindf_pdf_obj *temp = NULL;
			pindf_vector_index(dict->values, i, &temp);
			pindf_pdf_obj_destroy(temp);
			free(temp);
		}
		pindf_vector_destroy(dict->values);
		free(dict->values);
	}
}

void pindf_pdf_stream_destroy(pindf_pdf_stream *stream)
{
	if (stream->dict != NULL) {
		pindf_pdf_obj_destroy(stream->dict);
		free(stream->dict);
	}

	if (stream->stream_content != NULL) {
		pindf_uchar_str_destroy(stream->stream_content);
		free(stream->stream_content);
	}
}

void pindf_pdf_ind_obj_destroy(pindf_pdf_ind_obj *ind_obj)
{
	if (ind_obj->obj != NULL) {
		pindf_pdf_obj_destroy(ind_obj->obj);
		free(ind_obj->obj);
	}
}

void pindf_pdf_obj_destroy(pindf_pdf_obj *obj)
{
	switch (obj->obj_type) {
	case PINDF_PDF_ARRAY:
		if (obj->content.array != NULL) {
			for (int i = 0; i < obj->content.array->len; ++i) {
				pindf_pdf_obj *temp = NULL;
				pindf_vector_index(obj->content.array, i, &temp);
				pindf_pdf_obj_destroy(temp);
				free(temp);
			}
			pindf_vector_destroy(obj->content.array);
			free(obj->content.array);
		}
		break;
	case PINDF_PDF_DICT:
		pindf_pdf_dict_destory(&obj->content.dict);
		break;
	case PINDF_PDF_IND_OBJ:
		pindf_pdf_ind_obj_destroy(&obj->content.indirect_obj);
		break;
	case PINDF_PDF_LTR_STR:
		pindf_uchar_str_destroy(obj->content.ltr_str);
		free(obj->content.ltr_str);
		break;
	case PINDF_PDF_HEX_STR:
		pindf_uchar_str_destroy(obj->content.hex_str);
		free(obj->content.hex_str);
		break;
	case PINDF_PDF_STREAM:
		pindf_pdf_stream_destroy(&obj->content.stream);
		break;
	case PINDF_PDF_REF:
	case PINDF_PDF_INT:
	case PINDF_PDF_REAL:
		break;
	default:
		PINDF_WARN("invalid pdf obj_type");
	}
}