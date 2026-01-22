#include "doc.h"

struct pindf_doc *pindf_doc_new(const char *default_version)
{
	struct pindf_doc *doc = (struct pindf_doc*)malloc(sizeof(struct pindf_doc));
	
	doc->pdf_version = default_version;
	doc->fp = NULL;
	doc->xref_offset = 0;

	doc->ind_obj_list = pindf_vector_new(500, sizeof(struct pindf_obj_entry), NULL);
	
	doc->trailer = NULL;
	doc->xref = NULL;
	doc->xref_stream = NULL;

	return doc;
}

struct pindf_xref_table *pindf_xref_table_new(size_t obj_num, size_t len)
{
	struct pindf_xref_table *table = (struct pindf_xref_table*)malloc(sizeof(struct pindf_xref_table));
	table->obj_num = obj_num;
	table->len = len;
	table->prev_table = NULL;
	table->entries = (struct pindf_xref_entry*)calloc(len+1, sizeof(struct pindf_xref_entry));
	return table;
}

void pindf_xref_table_setentry(struct pindf_xref_table *table, uint index, uint64 offset, uint gen, int nf)
{
	table->entries[index] = (struct pindf_xref_entry){
		.offset = offset,
		.gen = gen,
		.nf = nf
	};
}

struct pindf_xref_entry *pindf_xref_table_getentry(struct pindf_xref_table *table, uint index) {
	assert(index < table->len);
	return &table->entries[index];
}

void pindf_doc_obj_setentry(struct pindf_doc *doc, struct pindf_pdf_obj *obj, uint64 offset)
{
	assert(obj->obj_type == PINDF_PDF_IND_OBJ);
	struct pindf_obj_entry entry = {
		.ind_obj = obj,
		.number = obj->content.indirect_obj.obj_num,
		.offset = offset,
	};
	pindf_vector_append(doc->ind_obj_list, &entry);
}

struct pindf_pdf_obj *pindf_doc_obj_getentry(struct pindf_doc *doc, uint64 obj_num)
{
	struct pindf_obj_entry *beg = (struct pindf_obj_entry *)doc->ind_obj_list->buf;
	struct pindf_obj_entry *end = beg + doc->ind_obj_list->len;

	struct pindf_pdf_obj *obj = NULL;
	for (struct pindf_obj_entry *p = beg; p != end; p++) {
		if (p->number == obj_num) {
			obj = p->ind_obj;
			break;
		}
	}
	return obj;
}