#pragma once

#include "obj.h"
#include "../container/simple_vector.h"
#include "../container/uchar_str.h"

struct pindf_xref_entry;
struct pindf_xref_table;

struct pindf_xref_entry {
	uint64 offset;
	uint gen;
	
	// 0 - n
	// 1 - f
	int nf;
};

struct pindf_xref_table {
	struct pindf_xref_entry *entries; // list of entries
	size_t obj_num;
	size_t len;
	struct pindf_xref_table *prev_table;
};

struct pindf_obj_entry {
	uint64 offset;
	uint number;
	struct pindf_pdf_obj *ind_obj;
};

struct pindf_doc {
	const char *pdf_version;
	struct pindf_vector *ind_obj_list; // array of ind_obj_entry
	int xref_offset;

	struct pindf_pdf_obj *trailer;
	struct pindf_xref_table *xref;

	struct pindf_pdf_obj *xref_stream;

	FILE *fp;
};

struct pindf_doc *pindf_doc_new(const char *default_version);
struct pindf_xref_table *pindf_xref_table_new(size_t obj_num, size_t len);
void pindf_xref_table_setentry(struct pindf_xref_table *table, uint index, uint64 offset, uint gen, int nf);
struct pindf_xref_entry *pindf_xref_table_getentry(struct pindf_xref_table *table, uint index);

void pindf_doc_obj_setentry(struct pindf_doc *doc, struct pindf_pdf_obj *obj, uint64 offset);
struct pindf_pdf_obj *pindf_doc_obj_getentry(struct pindf_doc *doc, uint64 obj_num);