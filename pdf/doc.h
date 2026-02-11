#pragma once

#include <stdbool.h>
#include "xref.h"
#include "obj.h"
#include "modif.h"


/// @brief pdf document
typedef struct {
	/// pdf version, like "%PDF-1.5"
	const char *pdf_version;
	/// array of ind_obj_entry
	pindf_obj_entry *ind_obj_list;
	/// offset of xref table in pdf file
	int xref_offset;

	/// trailer dictionary
	pindf_pdf_dict trailer;
	/// xref table
	pindf_xref *xref;

	pindf_modif *modif;

	/// underlying pdf file pointer
	FILE *fp;
} pindf_doc;

/// @brief create a new document
/// @param default_version default pdf version if version number not present in the file
/// @param fp underlying pdf file pointer
pindf_doc *pindf_doc_new(const char *default_version, FILE *fp);