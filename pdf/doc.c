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

/// @brief dump modification to a new files
/// 1. append new indirect object definitions
/// 2. append new xref table
/// 3. append new trailer
/// 4. append new startxref
void pindf_doc_save_modif(pindf_doc *doc, FILE *fp, bool compress_xref)
{
	assert(doc != NULL);
	assert(doc->xref_offset > 0);

	if (doc->modif == NULL) {
		PINDF_WARN("no modification");
		return;
	}

	fprintf(fp, "%%%%PDF-1.5\r\n");

	// 1
	// TODO: compress object
	pindf_ind_obj_node *node = doc->modif->modif_log->next;
	while (node) {
		size_t offset = ftell(fp);
		node->ind_obj->start_pos = offset;
		pindf_ind_obj_serialize_file(node->ind_obj, fp);
		node = node->next;
	}

	size_t xref_offset = ftell(fp);
	// 2
	if (!compress_xref){
		// xref table
		fprintf(fp, "xref\n");
		node = doc->modif->modif_log->next;

		int section_obj_num = -1, section_len = 0;
		pindf_ind_obj_node *first_node = node;
		while (node) {
			// find continuous regions
			pindf_pdf_ind_obj *ind_obj = node->ind_obj;
			if (ind_obj->obj_num != section_obj_num + section_len) {
				// write xref section header
				if (section_obj_num != -1) {
					fprintf(fp, "%d %d\r\n", section_obj_num, section_len);
					while (first_node != node) {
						// write xref table
						pindf_pdf_ind_obj *ind_obj = first_node->ind_obj;
						if (ind_obj == NULL) {
							// free object
							fprintf(fp, "0000000000 65535 f \r\n");
						} else {
							fprintf(fp, "%010zu %05d n\r\n", ind_obj->start_pos, ind_obj->generation_num);
						}
						first_node = first_node->next;
					}
				}
				first_node = node;
				section_obj_num = ind_obj->obj_num;
				section_len = 1;
			} else {
				section_len++;
			}
			node = node->next;
		}

		if (section_len > 0) {
			// write xref section header
			fprintf(fp, "%d %d\r\n", section_obj_num, section_len);
			while (first_node != node) {
				// write xref table
				pindf_pdf_ind_obj *ind_obj = first_node->ind_obj;
				if (ind_obj == NULL) {
					// free object
					fprintf(fp, "0000000000 65535 f \r\n");
				} else {
					fprintf(fp, "%010zu %05d n\r\n", ind_obj->start_pos, ind_obj->generation_num);
				}
				first_node = first_node->next;
			}
		}

		fprintf(fp, "trailer\r\n");
		pindf_pdf_dict new_trailer;
		pindf_dict_deepcopy(&doc->trailer, &new_trailer);

		// Set new prev and size
		// Prev: offset of previous xref table
		if (doc->xref_offset <= 0) {
			PINDF_ERR("no previous xref table (xref_offset = %d)", doc->xref_offset);
			pindf_pdf_dict_destory(&new_trailer);
			return;
		}

		pindf_pdf_obj *prev_obj = pindf_pdf_obj_new(PINDF_PDF_INT);
		prev_obj->content.num = doc->xref_offset;
		pindf_dict_set_value2(&new_trailer, "/Prev", prev_obj);

		// Size is max_obj_num
		pindf_pdf_obj *size_obj = pindf_pdf_obj_new(PINDF_PDF_INT);
		// !IMPORTANT must be one plus max_obj_num
		size_obj->content.num = doc->modif->max_obj_num + 1;
		pindf_dict_set_value2(&new_trailer, "/Size", size_obj);

		// del useless kv pairs
		pindf_dict_set_value2(&new_trailer, "/Filter", NULL);
		pindf_dict_set_value2(&new_trailer, "/Index", NULL);
		pindf_dict_set_value2(&new_trailer, "/W", NULL);
		pindf_dict_set_value2(&new_trailer, "/Type", NULL);
		pindf_dict_set_value2(&new_trailer, "/DecodeParms", NULL);
		pindf_dict_set_value2(&new_trailer, "/Length", NULL);

		pindf_dict_serialize_file(&new_trailer, fp);

		pindf_pdf_dict_destory(&new_trailer);
	} else {
		// compress xref table to xrefa stream
		PINDF_ERR("compress xref table not implemented");
	}
	fprintf(fp, "\r\nstartxref\r\n");
	fprintf(fp, "%zu\r\n", xref_offset);
	fprintf(fp, "%%%%EOF\n");
}