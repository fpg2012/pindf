#pragma once

#include "obj.h"
#include "../container/simple_vector.h"
#include "../container/uchar_str.h"

typedef struct {
	uint64 offset;
	uint gen;
	
	// 0 - n
	// 1 - f
	int nf;
} pindf_xref_entry;

typedef struct pindf_xref_table pindf_xref_table;

struct pindf_xref_table {
	pindf_xref_entry *entries; // list of entries
	size_t obj_num;
	size_t len;
	pindf_xref_table *prev_table;
};

struct pindf_obj_entry {
	uint64 offset;
	uint number;
	pindf_pdf_obj *ind_obj;
};

typedef struct {
	const char *pdf_version;
	pindf_vector *ind_obj_list; // array of ind_obj_entry
	int xref_offset;

	pindf_pdf_dict trailer;
	pindf_xref_table xref;

	// pindf_pdf_obj *xref_stream;

	FILE *fp;
} pindf_doc;

pindf_doc *pindf_doc_new(const char *default_version, FILE *fp);
void pindf_xref_table_init(pindf_xref_table *table, size_t obj_num, size_t len);
void pindf_xref_table_setentry(pindf_xref_table *table, uint index, uint64 offset, uint gen, int nf);
pindf_xref_entry *pindf_xref_table_getentry(pindf_xref_table *table, uint index);

void pindf_doc_obj_setentry(pindf_doc *doc, pindf_pdf_obj *obj, uint64 offset);
pindf_pdf_obj *pindf_doc_obj_getentry(pindf_doc *doc, uint64 obj_num, uint64 *offset);
pindf_pdf_obj *pindf_doc_getobj(pindf_doc *doc, uint64 obj_num);