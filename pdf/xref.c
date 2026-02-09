#include "xref.h"

void pindf_xref_section_init(pindf_xref_section *table, size_t obj_num, size_t len)
{
	table->obj_num = obj_num;
	table->len = len;
	table->entries = (pindf_xref_entry*)calloc(len+1, sizeof(pindf_xref_entry));
	table->next_table = NULL;
}

void pindf_xref_section_setentry(pindf_xref_section *table, uint index, uint64 offset, uint gen, int nf)
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

pindf_xref_entry *pindf_xref_section_getentry(pindf_xref_section *table, uint index) {
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

pindf_xref_section *pindf_xref_alloc_section(pindf_xref *xref, size_t obj_num, size_t len)
{
	assert(xref != NULL && "xref is null");
	assert(obj_num + len <= xref->size  && "xref overflow");

	pindf_xref_section *section = (pindf_xref_section*)malloc(sizeof(pindf_xref_section));
	*section = (pindf_xref_section){
		.obj_num = obj_num,
		.len = len,
		.entries = xref->entries + obj_num,
		.next_table = NULL,
	};

	if (xref->first_section == NULL) {
		xref->first_section = section;
	} else {
		pindf_xref_section *cur = xref->first_section;
		while (cur->next_table != NULL) {
			cur = cur->next_table;
		}
		cur->next_table = section;
	}

	xref->n_sections++;

	return section;
}