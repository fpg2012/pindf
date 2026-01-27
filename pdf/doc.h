#pragma once

#include "../type.h"
#include "obj.h"
#include "../container/simple_vector.h"

// #define PINDF_MAX_XREF_SECTIONS 512

enum pindf_xref_entry_type {
	PINDF_XREF_ENTRY_F = 0, // free
	PINDF_XREF_ENTRY_N, // normal
	PINDF_XREF_ENTRY_C, // compressed
};

typedef struct {
	enum pindf_xref_entry_type type;

	uint64 fields[2];
} pindf_xref_entry;

typedef struct pindf_xref_table pindf_xref_table;

struct pindf_xref_table {
	pindf_xref_entry *entries; // list of entries
	size_t obj_num;
	size_t len;

	pindf_xref_table *next_table;
};

struct pindf_obj_entry {
	uint64 offset;
	uint number;
	pindf_pdf_obj *ind_obj;
};

typedef struct pindf_xref pindf_xref;

typedef struct pindf_xref {
	pindf_xref_entry *entries; // flattened entries
	pindf_xref_table *first_section;

	size_t n_sections;
	size_t size;
} pindf_xref;

typedef struct {
	const char *pdf_version;
	pindf_vector *ind_obj_list; // array of ind_obj_entry
	int xref_offset;

	pindf_pdf_dict trailer;
	pindf_xref *xref;

	FILE *fp;
} pindf_doc;

pindf_doc *pindf_doc_new(const char *default_version, FILE *fp);

void pindf_xref_table_init(pindf_xref_table *table, size_t obj_num, size_t len);
void pindf_xref_table_setentry(pindf_xref_table *table, uint index, uint64 offset, uint gen, int nf);
pindf_xref_entry *pindf_xref_table_getentry(pindf_xref_table *table, uint index);

void pindf_xref_init(pindf_xref *xref, size_t size);
// void pindf_xref_addsection(pindf_xref *xref, pindf_xref_table section);
pindf_xref_entry *pindf_xref_getentry(pindf_xref *table, pindf_pdf_obj *ref);
pindf_xref_table *pindf_xref_alloc_section(pindf_xref *xref, size_t obj_num, size_t len);

void pindf_doc_obj_setentry(pindf_doc *doc, pindf_pdf_obj *obj, uint64 offset);
pindf_pdf_obj *pindf_doc_obj_getentry(pindf_doc *doc, uint64 obj_num, uint64 *offset);
pindf_pdf_obj *pindf_doc_getobj(pindf_doc *doc, uint64 obj_num);