#pragma once
#include "obj.h"

typedef struct pindf_ind_obj_node {
	uint obj_num;
	pindf_pdf_ind_obj *ind_obj;
	struct pindf_ind_obj_node *next;
} pindf_ind_obj_node;

/// @brief modification of a pdf doc
/// Modification is basically a list of indirect objects,
/// one doc can only have at most one modification.
/// When apply the modification to the file, it should be converted to:
/// 
/// 1. ind objects definitions
/// 2. xref sections
/// 3. a new trailer dictionary
///
/// and then, append to the end of pdf file
typedef struct {
	/// a linked-list of indirect objects.
	/// 
	pindf_ind_obj_node *modif_log;
	int count;
} pindf_modif;

void pindf_modif_init(pindf_modif *modif);
void pindf_modif_destroy(pindf_modif *modif);

pindf_modif *pindf_modif_new();

/// @brief add a new ind_obj to modification log
/// @param ind_obj the indirect object to add
/// @param obj_num obj_num of the indirect object, should be the same to `ind_obj->obj_num`
/// @note This functions transfers the ownership of `ind_obj` to `modif`. Caller must not use `ind_obj` after calling this function.
void pindf_modif_addentry(pindf_modif *modif, pindf_pdf_ind_obj *ind_obj, uint obj_num);
