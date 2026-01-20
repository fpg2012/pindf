#pragma once

#include "pdf_obj.h"
#include "simple_vector.h"
#include "uchar_str.h"

struct pindf_xref_entry;
struct pindf_xref_table;

struct pindf_xref_entry {
	uint64 offset;
	uint generation_num;
	
	// 0 - n
	// 1 - f
	int nf;
};

struct pindf_xref_table {
	struct pindf_xref_entry *entries;
	size_t len;
	struct pindf_xref_table *next_table;
};

struct pindf_pdf_doc {
	const char *pdf_version;
	struct pindf_vector *body_obj_list;
	struct pindf_vector *ind_obj_list;
	struct pindf_pdf_obj *trailer;
	int xref_offset;

	FILE *fp;
};

struct pindf_pdf_doc *pindf_pdf_doc_new(const char *default_version);