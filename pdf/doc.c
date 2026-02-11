#include "doc.h"
#include "../logger/logger.h"
#include "../core/serialize.h"
#include "obj.h"

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

	doc->modif = NULL;

	return doc;
}