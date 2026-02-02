#include "doc.h"

pindf_doc *pindf_doc_new(const char *default_version, FILE *fp)
{
	pindf_doc *doc = (pindf_doc*)malloc(sizeof(pindf_doc));
	
	doc->pdf_version = default_version;
	doc->fp = NULL;
	doc->xref_offset = 0;

	// doc->ind_obj_list = pindf_vector_new(500, sizeof(struct pindf_obj_entry), NULL);
	doc->ind_obj_list = NULL;

	doc->xref = NULL;

	doc->fp = fp;

	return doc;
}

void pindf_xref_table_init(pindf_xref_table *table, size_t obj_num, size_t len)
{
	table->obj_num = obj_num;
	table->len = len;
	table->entries = (pindf_xref_entry*)calloc(len+1, sizeof(pindf_xref_entry));
	table->next_table = NULL;
}

void pindf_xref_table_setentry(pindf_xref_table *table, uint index, uint64 offset, uint gen, int nf)
{
	if (table->entries[index].type == 0 &&
		table->entries[index].fields[0] == 0
		&& table->entries[index].fields[0] == 0
	) {
		table->entries[index] = (pindf_xref_entry){
			.fields = {offset, gen,},
			.type = nf,
		};
	}
	
}

pindf_xref_entry *pindf_xref_table_getentry(pindf_xref_table *table, uint index) {
	assert(index < table->len);
	return &table->entries[index];
}


/// xref ///
void pindf_xref_init(pindf_xref *xref, size_t size)
{
	xref->n_sections = 0;
	xref->size = size;
	
	xref->entries = (pindf_xref_entry*)calloc(size, sizeof(pindf_xref_entry));
	xref->first_section = NULL;
}

pindf_xref_table *pindf_xref_alloc_section(pindf_xref *xref, size_t obj_num, size_t len)
{
	assert(xref != NULL && "xref is null");
	assert(obj_num + len <= xref->size  && "xref overflow");

	pindf_xref_table *section = (pindf_xref_table*)malloc(sizeof(pindf_xref_table));
	*section = (pindf_xref_table){
		.obj_num = obj_num,
		.len = len,
		.entries = xref->entries + obj_num,
		.next_table = NULL,
	};

	if (xref->first_section == NULL) {
		xref->first_section = section;
	} else {
		pindf_xref_table *cur = xref->first_section;
		while (cur->next_table != NULL) {
			cur = cur->next_table;
		}
		cur->next_table = section;
	}

	xref->n_sections++;

	return section;
}