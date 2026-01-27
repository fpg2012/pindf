#include "obj.h"

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
			fprintf(stderr, "[error] non-name dict key\n");
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
			fprintf(stderr, "[error] non-name dict key\n");
			return NULL;
		}
		if (pindf_uchar_str_cmp2(cur_key->content.name, (const uchar*)key, len) == 0) {
			break;
		}
	}

	pindf_vector_index(dict->values, index, &value);
	return value;
}