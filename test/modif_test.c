#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../pdf/modif.h"

static pindf_pdf_ind_obj *make_ind_obj(int obj_num)
{
	pindf_pdf_ind_obj *ind = (pindf_pdf_ind_obj*)malloc(sizeof(pindf_pdf_ind_obj));
	*ind = (pindf_pdf_ind_obj){
		.obj_num = obj_num,
		.generation_num = 0,
		.obj = NULL,
		.start_pos = 0,
	};
	return ind;
}

static pindf_ind_obj_node *find_node(pindf_modif *modif, int obj_num)
{
	pindf_ind_obj_node *node = modif->modif_log->next;
	while (node) {
		if (node->obj_num == (uint)obj_num) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

int main(void)
{
	pindf_modif modif;
	pindf_modif_init(&modif);

	pindf_pdf_ind_obj *obj5a = make_ind_obj(5);
	pindf_pdf_ind_obj *obj2 = make_ind_obj(2);
	pindf_pdf_ind_obj *obj8 = make_ind_obj(8);

	pindf_modif_addentry(&modif, obj5a, 5);
	pindf_modif_addentry(&modif, obj2, 2);
	pindf_modif_addentry(&modif, obj8, 8);

	pindf_pdf_ind_obj *obj5b = make_ind_obj(5);
	pindf_modif_addentry(&modif, obj5b, 5);

	int expected[] = {2, 5, 8};
	int idx = 0;
	for (pindf_ind_obj_node *node = modif.modif_log->next; node; node = node->next) {
		assert(node->obj_num == (uint)expected[idx]);
		idx++;
	}
	assert(idx == 3);

	pindf_ind_obj_node *node5 = find_node(&modif, 5);
	assert(node5 != NULL);
	assert(node5->ind_obj == obj5b);

	pindf_modif_destroy(&modif);

	printf("modif_test passed\n");
	return 0;
}
