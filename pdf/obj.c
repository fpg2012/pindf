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

void pindf_arr_deepcopy(const pindf_vector *arr, pindf_vector *dst)
{
	if (arr == NULL || dst == NULL) return;

	dst->elem_size = sizeof(pindf_pdf_obj*);
	dst->len = 0;
	dst->capacity = arr->capacity;
	dst->buf = (uchar*)malloc(dst->capacity * dst->elem_size);

	for (size_t i = 0; i < arr->len; i++) {
		pindf_pdf_obj *elem = NULL;
		pindf_vector_index((pindf_vector*)arr, i, &elem);
		pindf_pdf_obj *elem_copy = pindf_pdf_obj_deepcopy(elem);
		pindf_vector_append(dst, &elem_copy);
	}
}

void pindf_dict_deepcopy(const pindf_pdf_dict *dict, pindf_pdf_dict *dst)
{
	if (dict == NULL || dst == NULL) return;

	dst->keys = pindf_vector_new(8, sizeof(pindf_pdf_obj*));
	dst->values = pindf_vector_new(8, sizeof(pindf_pdf_obj*));

	for (size_t i = 0; i < dict->keys->len; i++) {
		pindf_pdf_obj *key = NULL, *val = NULL;
		pindf_vector_index((pindf_vector*)dict->keys, i, &key);
		pindf_vector_index((pindf_vector*)dict->values, i, &val);

		pindf_pdf_obj *key_copy = pindf_pdf_obj_deepcopy(key);
		pindf_pdf_obj *val_copy = pindf_pdf_obj_deepcopy(val);

		pindf_vector_append(dst->keys, &key_copy);
		pindf_vector_append(dst->values, &val_copy);
	}
}

void pindf_ind_obj_deepcopy(const pindf_pdf_ind_obj *ind_obj, pindf_pdf_ind_obj *dst)
{
	if (ind_obj == NULL || dst == NULL) return;

	dst->obj_num = ind_obj->obj_num;
	dst->generation_num = ind_obj->generation_num;
	dst->start_pos = ind_obj->start_pos;
	dst->obj = pindf_pdf_obj_deepcopy(ind_obj->obj);
}

pindf_pdf_obj *pindf_pdf_obj_deepcopy(const pindf_pdf_obj *obj)
{
	if (obj == NULL) return NULL;

	pindf_pdf_obj *copy = pindf_pdf_obj_new(obj->obj_type);

	switch (obj->obj_type) {
	case PINDF_PDF_INT:
		copy->content.num = obj->content.num;
		break;

	case PINDF_PDF_REAL:
		copy->content.real_num = obj->content.real_num;
		break;

	case PINDF_PDF_BOOL:
		copy->content.boolean = obj->content.boolean;
		break;

	case PINDF_PDF_NULL:
		// nothing to copy
		break;

	case PINDF_PDF_NAME:
		copy->content.name = pindf_uchar_str_copy(obj->content.name);
		break;

	case PINDF_PDF_LTR_STR:
		copy->content.ltr_str = pindf_uchar_str_copy(obj->content.ltr_str);
		break;

	case PINDF_PDF_HEX_STR:
		copy->content.hex_str = pindf_uchar_str_copy(obj->content.hex_str);
		break;

	case PINDF_PDF_ARRAY:
		copy->content.array = pindf_vector_new(obj->content.array ? obj->content.array->len : 4, sizeof(pindf_pdf_obj*));
		pindf_arr_deepcopy(obj->content.array, copy->content.array);
		break;

	case PINDF_PDF_DICT:
		pindf_dict_deepcopy(&obj->content.dict, &copy->content.dict);
		break;

	case PINDF_PDF_STREAM: {
		const pindf_pdf_stream *src_stream = &obj->content.stream;
		pindf_pdf_stream *dst_stream = &copy->content.stream;

		dst_stream->content_offset = src_stream->content_offset;
		dst_stream->dict = pindf_pdf_obj_deepcopy(src_stream->dict);

		if (src_stream->stream_content != NULL) {
			dst_stream->stream_content = pindf_uchar_str_copy(src_stream->stream_content);
		} else {
			dst_stream->stream_content = NULL;
		}
		break;
	}

	case PINDF_PDF_REF:
		copy->content.ref = obj->content.ref;
		break;

	case PINDF_PDF_IND_OBJ:
		pindf_ind_obj_deepcopy(&obj->content.indirect_obj, &copy->content.indirect_obj);
		break;

	default:
		PINDF_ERR("unknown pdf obj type in deepcopy: %d", obj->obj_type);
		free(copy);
		return NULL;
	}

	return copy;
}

void pindf_pdf_obj_destroy(pindf_pdf_obj *obj)
{
	if (obj == NULL)
		return;
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
	case PINDF_PDF_NAME:
		pindf_uchar_str_destroy(obj->content.name);
		free(obj->content.name);
		break;
	case PINDF_PDF_STREAM:
		pindf_pdf_stream_destroy(&obj->content.stream);
		break;
	case PINDF_PDF_REF:
	case PINDF_PDF_INT:
	case PINDF_PDF_REAL:
	case PINDF_PDF_BOOL:
	case PINDF_PDF_NULL:
		break;
	default:
		PINDF_WARN("invalid pdf obj_type");
	}
}

void pindf_dict_set_value(pindf_pdf_dict *dict, const uchar *key, size_t key_len, pindf_pdf_obj *value)
{
	assert(dict != NULL);
	assert(key != NULL);
	assert(dict->keys != NULL);

	// Check if key already exists
	pindf_pdf_obj **beg = (pindf_pdf_obj **)dict->keys->buf;
	pindf_pdf_obj **end = beg + dict->keys->len;

	size_t index = 0;
	for (pindf_pdf_obj **p = beg; p != end; ++p, ++index) {
		pindf_pdf_obj *cur_key = *p;
		if (cur_key->obj_type != PINDF_PDF_NAME) {
			PINDF_ERR("non-name dict key");
			return;
		}
		if (pindf_uchar_str_cmp2(cur_key->content.name, key, key_len) == 0) {
			// Key found, replace the value at the same index
			// Get pointer to the old value in the vector
			pindf_pdf_obj **value_ptr = (pindf_pdf_obj **)(dict->values->buf + index * dict->values->elem_size);
			pindf_pdf_obj *old_value = *value_ptr;

			// Destroy the old value if it exists and is different from new value
			if (old_value != NULL && old_value != value) {
				pindf_pdf_obj_destroy(old_value);
				free(old_value);
			}

			// Set the new value
			*value_ptr = value;
			return;
		}
	}

	// Key not found, create new name object
	pindf_pdf_obj *name_obj = pindf_pdf_obj_new(PINDF_PDF_NAME);
	if (name_obj == NULL) {
		PINDF_ERR("failed to create name object");
		return;
	}

	// Create uchar string for the key
	pindf_uchar_str *key_str = pindf_uchar_str_from_cstr((const char *)key, key_len);
	if (key_str == NULL) {
		PINDF_ERR("failed to create key string");
		free(name_obj);
		return;
	}

	name_obj->content.name = key_str;

	// Add key-value pair
	pindf_vector_append(dict->keys, &name_obj);
	pindf_vector_append(dict->values, &value);
}

void pindf_dict_set_value2(pindf_pdf_dict *dict, const char *key, pindf_pdf_obj *value)
{
	assert(dict != NULL);
	assert(key != NULL);

	size_t key_len = strlen(key);
	pindf_dict_set_value(dict, (const uchar *)key, key_len, value);
}

void pindf_pdf_dict_init(pindf_pdf_dict *dict)
{
	assert(dict != NULL);
	dict->keys = pindf_vector_new(8, sizeof(pindf_pdf_obj*));
	dict->values = pindf_vector_new(8, sizeof(pindf_pdf_obj*));
}