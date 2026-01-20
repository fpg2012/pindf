#include "pdf_doc.h"
#include "pdf_obj.h"
#include "simple_vector.h"

struct pindf_pdf_doc *pindf_pdf_doc_new(const char *default_version)
{
	struct pindf_pdf_doc *doc = (struct pindf_pdf_doc*)malloc(sizeof(struct pindf_pdf_doc));
	
	doc->pdf_version = default_version;
	doc->fp = NULL;
	doc->xref_offset = 0;

	doc->body_obj_list = pindf_vector_new(100, sizeof(struct pindf_pdf_obj*), NULL);
	doc->ind_obj_list = pindf_vector_new(500, sizeof(struct pindf_pdf_obj*), NULL);
	
	doc->trailer = NULL;

	return doc;
}