#include "doc.h"

pindf_doc *pindf_doc_new(const char *default_version, FILE *fp)
{
	pindf_doc *doc = (pindf_doc*)malloc(sizeof(pindf_doc));
	
	doc->pdf_version = default_version;
	doc->fp = NULL;
	doc->xref_offset = 0;

	doc->ind_obj_list = pindf_vector_new(500, sizeof(struct pindf_obj_entry), NULL);
	
	// doc->trailer = NULL;
	// doc->xref = NULL;
	// doc->xref_stream = NULL;

	doc->fp = fp;

	return doc;
}

void pindf_xref_table_init(pindf_xref_table *table, size_t obj_num, size_t len)
{
	table->obj_num = obj_num;
	table->len = len;
	table->prev_table = NULL;
	table->entries = (pindf_xref_entry*)calloc(len+1, sizeof(pindf_xref_entry));
}

void pindf_xref_table_setentry(pindf_xref_table *table, uint index, uint64 offset, uint gen, int nf)
{
	table->entries[index] = (pindf_xref_entry){
		.fields = {offset, gen,},
		.type = nf,
	};
}

pindf_xref_entry *pindf_xref_table_getentry(pindf_xref_table *table, uint index) {
	assert(index < table->len);
	return &table->entries[index];
}

void pindf_doc_obj_setentry(pindf_doc *doc, pindf_pdf_obj *obj, uint64 offset)
{
	assert(obj->obj_type == PINDF_PDF_IND_OBJ);
	struct pindf_obj_entry entry = {
		.ind_obj = obj,
		.number = obj->content.indirect_obj.obj_num,
		.offset = offset,
	};
	pindf_vector_append(doc->ind_obj_list, &entry);
}

pindf_pdf_obj *pindf_doc_obj_getentry(pindf_doc *doc, uint64 obj_num, uint64 *offset)
{
	struct pindf_obj_entry *beg = (struct pindf_obj_entry *)doc->ind_obj_list->buf;
	struct pindf_obj_entry *end = beg + doc->ind_obj_list->len;

	pindf_pdf_obj *obj = NULL;
	for (struct pindf_obj_entry *p = beg; p != end; p++) {
		if (p->number == obj_num) {
			obj = p->ind_obj;
			*offset = p->offset;
			break;
		}
	}
	return obj;
}

pindf_pdf_obj *pindf_doc_getobj(pindf_doc *doc, uint64 obj_num)
{
	assert(doc != NULL);
	return NULL;
}