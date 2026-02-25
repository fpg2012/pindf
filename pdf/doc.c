#include "doc.h"
#include "../logger/logger.h"
#include "../core/serialize.h"
#include "obj.h"
#include "xref.h"

pindf_doc *pindf_doc_new(const char *default_version, FILE *fp)
{
	pindf_doc *doc = (pindf_doc*)malloc(sizeof(pindf_doc));
	
	doc->pdf_version = default_version;
	doc->xref_offset = 0;

	// doc->ind_obj_list = pindf_vector_new(500, sizeof(struct pindf_obj_entry), NULL);
	doc->ind_obj_list = NULL;

	doc->xref = NULL;

	doc->fp = fp;

	doc->modif = NULL;

	return doc;
}

void pindf_doc_destroy(pindf_doc *doc) {
	if (doc == NULL)
		return;
	
	if (doc->modif != NULL) {
		pindf_modif_destroy(doc->modif);
		free(doc->modif);
		doc->modif = NULL;
	}

	if (doc->ind_obj_list != NULL) {
		for (int i = 0; i < doc->xref->size; i++) {
			pindf_obj_entry_destroy(&doc->ind_obj_list[i]);
		}
		free(doc->ind_obj_list);
		doc->ind_obj_list = NULL;
	}
	
	if (doc->xref != NULL) {
		pindf_xref_destroy(doc->xref);
		free(doc->xref);
		doc->xref = NULL;
	}

	if (doc->fp != NULL)
		fclose(doc->fp);
}