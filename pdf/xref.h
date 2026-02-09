#pragma once

#include <stdlib.h>
#include "../type.h"
#include "obj.h"

enum pindf_xref_entry_type {
	/// free object
	PINDF_XREF_ENTRY_F = 0,
	/// normal object
	PINDF_XREF_ENTRY_N,
	/// compressed object
	PINDF_XREF_ENTRY_C,
};

/// @brief For lazy loading of objects
enum pindf_obj_avail {
	PINDF_OBJ_NOTLOAD = 0,
	PINDF_OBJ_AVAILABLE,
	PINDF_OBJ_FREE,
};

/// @brief An entry of xref table
/// 
/// - for N entry (free object), fields[0]=offset, fields[1]=gen number
/// - for C entry (compressed object), fields[0]=stream obj num, fields[1]=index in stream
typedef struct {
	enum pindf_xref_entry_type type;

	uint64 fields[2];
} pindf_xref_entry;

typedef struct pindf_xref_section pindf_xref_section;

/// @brief xref section (part of the entire xref table)
/// 
/// xref section do not own its entries list. Instead, entries is just a reference
struct pindf_xref_section {
	/// array of entries, a continuous memory space reference to the real xref table
	pindf_xref_entry *entries;
	/// obj num of the first entry
	size_t obj_num;
	/// number of objects
	size_t len;

	pindf_xref_section *next_table;
};

/// @brief An entry of indirected obj
/// 
/// each obj entry corresponds to an entry in xref table
/// it owns the ind_obj
typedef struct {
	int available;
	uint64 offset;
	uint number;
	pindf_pdf_obj *ind_obj;
} pindf_obj_entry;

typedef struct pindf_xref pindf_xref;

/// @brief xref table
typedef struct pindf_xref {
	/// flattened entries
	pindf_xref_entry *entries;
	/// linked-list of sections
	pindf_xref_section *first_section;

	/// number of sections
	size_t n_sections;
	/// number of objects (maximum of all obj nums)
	size_t size;
} pindf_xref;


/// @brief init xref section
void pindf_xref_section_init(pindf_xref_section *table, size_t obj_num, size_t len);

/// @param index index in this section, so the real obj_num=index + table->obj_num
/// @param offset offset in pdf file
/// @param gen generation number, usually 0
/// @param nf free or normal
void pindf_xref_section_setentry(pindf_xref_section *table, uint index, uint64 offset, uint gen, int nf);

pindf_xref_entry *pindf_xref_section_getentry(pindf_xref_section *table, uint index);

void pindf_xref_init(pindf_xref *xref, size_t size);

pindf_xref_entry *pindf_xref_getentry(pindf_xref *table, pindf_pdf_obj *ref);

/// @brief allocation a continous memory in xref table for a section
/// this should be the only function to create xref section struct
pindf_xref_section *pindf_xref_alloc_section(pindf_xref *xref, size_t obj_num, size_t len);