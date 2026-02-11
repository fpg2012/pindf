#include "modif.h"
#include "obj.h"
#include <stdbool.h>

static void _init_node(pindf_ind_obj_node *node)
{
	*node = (pindf_ind_obj_node){
		.ind_obj = NULL,
		.next = NULL,
		.obj_num = 0,
	};
}

static pindf_ind_obj_node *_new_node()
{
	pindf_ind_obj_node *node = malloc(sizeof(pindf_ind_obj_node));
	_init_node(node);
	return node;
}

static void _destroy_node(pindf_ind_obj_node *node)
{
	if (node == NULL || node->ind_obj == NULL) {
		return;
	}
	pindf_pdf_ind_obj_destroy(node->ind_obj);
	free(node->ind_obj);
}

static void _free_node(pindf_ind_obj_node *node)
{
	_destroy_node(node);
	free(node);
}

void pindf_modif_init(pindf_modif *modif, int64 max_obj_num)
{
	assert(max_obj_num >= 1);
	*modif = (pindf_modif){
		.modif_log = _new_node(),
		.count = 0,
		.max_obj_num = max_obj_num - 1,
	};
}

void pindf_modif_destroy(pindf_modif *modif)
{
	pindf_ind_obj_node *temp = NULL;
	// free all nodes
	while (modif->modif_log->next) {
		temp = modif->modif_log->next;
		modif->modif_log->next = temp->next;
		_free_node(temp);
	}
	
	_free_node(modif->modif_log);
}

pindf_modif *pindf_modif_new(int64 max_obj_num)
{
	pindf_modif *modif = (pindf_modif*)malloc(sizeof(pindf_modif));
	pindf_modif_init(modif, max_obj_num);
	return modif;
}

void pindf_modif_addentry(pindf_modif *modif, pindf_pdf_ind_obj *ind_obj, uint obj_num)
{
	assert(ind_obj != NULL);
	assert(obj_num != 0 && ind_obj->obj_num == obj_num);

	// insert to appropriate pos (sorted by obj_num)
	pindf_ind_obj_node *prev = modif->modif_log;
	while (prev->next && prev->next->obj_num < obj_num) {
		prev = prev->next;
	}

	if (prev->next && prev->next->obj_num == obj_num) {
		// replace existing
		_destroy_node(prev->next);
		prev->next->ind_obj = ind_obj;
		prev->next->obj_num = obj_num;
	} else {
		// insert new node
		pindf_ind_obj_node *new_node = _new_node();
		*new_node = (pindf_ind_obj_node){
			.ind_obj = ind_obj,
			.next = prev->next,
			.obj_num = obj_num,
		};
		prev->next = new_node;
		modif->count++;

		if (obj_num > modif->max_obj_num) {
			modif->max_obj_num = obj_num;
		}
	}
}
