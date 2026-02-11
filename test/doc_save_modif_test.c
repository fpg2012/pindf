#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pindf.h"
#include "../logger/logger.h"
#include "../pdf/doc.h"
#include "../pdf/modif.h"
#include "../pdf/obj.h"

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

static pindf_pdf_obj *create_simple_dict(const char *type_name)
{
    pindf_pdf_obj *dict_obj = pindf_pdf_obj_new(PINDF_PDF_DICT);
    // Initialize dictionary vectors
    dict_obj->content.dict.keys = pindf_vector_new(8, sizeof(pindf_pdf_obj*));
    dict_obj->content.dict.values = pindf_vector_new(8, sizeof(pindf_pdf_obj*));

    pindf_pdf_obj *type_obj = pindf_pdf_obj_new(PINDF_PDF_NAME);
    type_obj->content.name = pindf_uchar_str_from_cstr(type_name, strlen(type_name));
    pindf_dict_set_value2(&dict_obj->content.dict, "/Type", type_obj);
    return dict_obj;
}

static pindf_pdf_obj *make_ref_obj(int obj_num, int gen_num)
{
    pindf_pdf_obj *ref = pindf_pdf_obj_new(PINDF_PDF_REF);
    ref->content.ref.obj_num = obj_num;
    ref->content.ref.generation_num = gen_num;
    return ref;
}

static pindf_pdf_obj *resolve_ref(
    pindf_doc *doc,
    pindf_parser *parser,
    pindf_lexer *lexer,
    pindf_pdf_obj *obj,
    pindf_pdf_ind_obj **out_ind_obj)
{
    if (out_ind_obj) *out_ind_obj = NULL;
    if (obj == NULL) return NULL;

    if (obj->obj_type == PINDF_PDF_REF) {
        pindf_pdf_obj *ind_obj = pindf_doc_getobj(doc, parser, lexer, obj->content.ref.obj_num);
        if (ind_obj == NULL) {
            PINDF_ERR("failed to resolve ref %d R", obj->content.ref.obj_num);
            return NULL;
        }
        if (ind_obj->obj_type != PINDF_PDF_IND_OBJ) {
            PINDF_ERR("failed to resolve ref %d R: not ind_obj, but %d", obj->content.ref.obj_num, ind_obj->obj_type);
        }
        if (out_ind_obj) *out_ind_obj = &ind_obj->content.indirect_obj;
        return ind_obj->content.indirect_obj.obj;
    }

    if (obj->obj_type == PINDF_PDF_IND_OBJ) {
        if (out_ind_obj) *out_ind_obj = &obj->content.indirect_obj;
        return obj->content.indirect_obj.obj;
    }

    return obj;
}

static pindf_pdf_obj *find_first_page_ref(
    pindf_doc *doc,
    pindf_parser *parser,
    pindf_lexer *lexer,
    pindf_pdf_obj *pages_dict)
{
    if (pages_dict == NULL || pages_dict->obj_type != PINDF_PDF_DICT) {
        return NULL;
    }

    pindf_pdf_obj *kids = pindf_dict_getvalue2(&pages_dict->content.dict, "/Kids");
    if (kids == NULL || kids->obj_type != PINDF_PDF_ARRAY || kids->content.array->len == 0) {
        return NULL;
    }

    pindf_pdf_obj *first = NULL;
    pindf_vector_index(kids->content.array, 0, &first);
    if (first == NULL) return NULL;

    if (first->obj_type == PINDF_PDF_REF) {
        pindf_pdf_ind_obj *ind = NULL;
        pindf_pdf_obj *resolved = resolve_ref(doc, parser, lexer, first, &ind);
        if (resolved && resolved->obj_type == PINDF_PDF_DICT) {
            pindf_pdf_obj *type = pindf_dict_getvalue2(&resolved->content.dict, "/Type");
            if (type && type->obj_type == PINDF_PDF_NAME &&
                pindf_uchar_str_cmp3(type->content.name, "/Page") == 0) {
                return first;
            }
            if (type && type->obj_type == PINDF_PDF_NAME &&
                pindf_uchar_str_cmp3(type->content.name, "/Pages") == 0) {
                return find_first_page_ref(doc, parser, lexer, resolved);
            }
        }
        return NULL;
    }

    if (first->obj_type == PINDF_PDF_IND_OBJ) {
        pindf_pdf_obj *resolved = first->content.indirect_obj.obj;
        if (resolved && resolved->obj_type == PINDF_PDF_DICT) {
            pindf_pdf_obj *type = pindf_dict_getvalue2(&resolved->content.dict, "/Type");
            if (type && type->obj_type == PINDF_PDF_NAME &&
                pindf_uchar_str_cmp3(type->content.name, "/Page") == 0) {
                return make_ref_obj(first->content.indirect_obj.obj_num,
                                    first->content.indirect_obj.generation_num);
            }
            if (type && type->obj_type == PINDF_PDF_NAME &&
                pindf_uchar_str_cmp3(type->content.name, "/Pages") == 0) {
                return find_first_page_ref(doc, parser, lexer, resolved);
            }
        }
        return NULL;
    }

    return NULL;
}

int main(int argc, const char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [pdf_filename] [output_filename]\n", argv[0]);
        return 1;
    }

    pindf_set_log_level(PINDF_LOG_LEVEL_INFO);
    pindf_lexer *lexer = pindf_lexer_new();
    pindf_parser *parser = pindf_parser_new();

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        perror("Failed to open input file");
        return 1;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    uint64 file_len = ftell(f);
    fseek(f, 0, SEEK_SET);

    printf("Input PDF file size: %llu bytes\n", file_len);
    assert(file_len > 0);

    // Parse the PDF file
    pindf_doc *doc = NULL;
    int ret = pindf_file_parse(parser, lexer, f, file_len, &doc);
    if (ret < 0) {
        fprintf(stderr, "Failed to parse PDF file\n");
        fclose(f);
        return 1;
    }

    printf("Parsed PDF successfully\n");
    printf("xref_offset: %d\n", doc->xref_offset);
    printf("trailer entries: %zu\n", doc->trailer.keys->len);

    // Create modification if not exists
    if (doc->modif == NULL) {
        // Estimate max object number from xref size
        int64 max_obj_num = doc->xref ? doc->xref->size : 0;
        doc->modif = pindf_modif_new(max_obj_num); // Add some extra space
    }

    // Add some test modifications
    printf("\nAdding modifications...\n");

    // Resolve Catalog and Pages
    int new_obj_num = 0;
    pindf_pdf_obj *root_ref = pindf_dict_getvalue2(&doc->trailer, "/Root");
    pindf_pdf_ind_obj *root_ind = NULL;
    pindf_pdf_obj *root_dict = resolve_ref(doc, parser, lexer, root_ref, &root_ind);
    if (root_dict == NULL || root_dict->obj_type != PINDF_PDF_DICT) {
        fprintf(stderr, "Failed to resolve /Root catalog dictionary\n");
        return 1;
    }

    pindf_pdf_obj *pages_ref = pindf_dict_getvalue2(&root_dict->content.dict, "/Pages");
    if (pages_ref == NULL) {
        fprintf(stderr, "Catalog /Pages is missing\n");
        return 1;
    }

    pindf_pdf_ind_obj *pages_ind = NULL;
    pindf_pdf_obj *pages_dict = resolve_ref(doc, parser, lexer, pages_ref, &pages_ind);
    if (pages_dict == NULL || pages_dict->obj_type != PINDF_PDF_DICT) {
        fprintf(stderr, "Failed to resolve /Pages dictionary\n");
        return 1;
    }

    // Find a page reference for outline destination
    pindf_pdf_obj *page_ref = find_first_page_ref(doc, parser, lexer, pages_dict);
    if (page_ref == NULL) {
        fprintf(stderr, "Failed to find a page to attach outline destination\n");
        return 1;
    }
    new_obj_num = doc->modif->max_obj_num;

    // Add a simple outline item (create /Outlines if missing)
    pindf_pdf_ind_obj *outlines_ind = NULL;
    pindf_pdf_obj *outlines_ref = pindf_dict_getvalue2(&root_dict->content.dict, "/Outlines");
    pindf_pdf_obj *outlines_dict = resolve_ref(doc, parser, lexer, outlines_ref, &outlines_ind);

    if (outlines_dict == NULL || outlines_dict->obj_type != PINDF_PDF_DICT || outlines_ind == NULL) {
        // create a new outlines root
        new_obj_num++;
        outlines_ind = make_ind_obj(new_obj_num);
        outlines_ind->obj = create_simple_dict("/Outlines");

        pindf_pdf_obj *count0 = pindf_pdf_obj_new(PINDF_PDF_INT);
        count0->content.num = 0;
        pindf_dict_set_value2(&outlines_ind->obj->content.dict, "/Count", count0);

        pindf_modif_addentry(doc->modif, outlines_ind, outlines_ind->obj_num);

        // attach /Outlines to catalog
        pindf_pdf_ind_obj *root_mod = make_ind_obj(root_ind->obj_num);
        root_mod->generation_num = root_ind->generation_num;
        root_mod->obj = pindf_pdf_obj_deepcopy(root_dict);
        pindf_dict_set_value2(&root_mod->obj->content.dict, "/Outlines",
            make_ref_obj(outlines_ind->obj_num, outlines_ind->generation_num));
        pindf_modif_addentry(doc->modif, root_mod, root_mod->obj_num);

        outlines_dict = outlines_ind->obj;
        printf("  Created Outlines root (obj %d)\n", outlines_ind->obj_num);
    }

    if (outlines_dict != NULL && outlines_dict->obj_type == PINDF_PDF_DICT && outlines_ind != NULL) {
        pindf_pdf_ind_obj *outlines_mod = make_ind_obj(outlines_ind->obj_num);
        outlines_mod->generation_num = outlines_ind->generation_num;
        outlines_mod->obj = pindf_pdf_obj_deepcopy(outlines_dict);

        // create new outline item
        new_obj_num++;
        pindf_pdf_ind_obj *outline_item = make_ind_obj(new_obj_num);
        outline_item->obj = pindf_pdf_obj_new(PINDF_PDF_DICT);
        outline_item->obj->content.dict.keys = pindf_vector_new(8, sizeof(pindf_pdf_obj*));
        outline_item->obj->content.dict.values = pindf_vector_new(8, sizeof(pindf_pdf_obj*));

        pindf_pdf_obj *title = pindf_pdf_obj_new(PINDF_PDF_LTR_STR);
        title->content.ltr_str = pindf_uchar_str_from_cstr("Random Outline", 14);
        pindf_dict_set_value2(&outline_item->obj->content.dict, "/Title", title);

        pindf_dict_set_value2(&outline_item->obj->content.dict, "/Parent",
            make_ref_obj(outlines_mod->obj_num, outlines_mod->generation_num));

        // Dest [page /XYZ 0 0 0]
        pindf_pdf_obj *dest = pindf_pdf_obj_new(PINDF_PDF_ARRAY);
        dest->content.array = pindf_vector_new(5, sizeof(pindf_pdf_obj*));
        pindf_pdf_obj *dest_page = NULL;
        if (page_ref->obj_type == PINDF_PDF_REF) {
            dest_page = make_ref_obj(page_ref->content.ref.obj_num, page_ref->content.ref.generation_num);
        } else if (page_ref->obj_type == PINDF_PDF_IND_OBJ) {
            dest_page = make_ref_obj(page_ref->content.indirect_obj.obj_num,
                                     page_ref->content.indirect_obj.generation_num);
        } else {
            // fallback: use the original object pointer
            dest_page = page_ref;
        }
        pindf_vector_append(dest->content.array, &dest_page);
        pindf_pdf_obj *xyz = pindf_pdf_obj_new(PINDF_PDF_NAME);
        xyz->content.name = pindf_uchar_str_from_cstr("/XYZ", 4);
        pindf_vector_append(dest->content.array, &xyz);
        for (int i = 0; i < 3; i++) {
            pindf_pdf_obj *num = pindf_pdf_obj_new(PINDF_PDF_INT);
            num->content.num = 0;
            pindf_vector_append(dest->content.array, &num);
        }
        pindf_dict_set_value2(&outline_item->obj->content.dict, "/Dest", dest);

        // Update outlines /First, /Last, /Count and link siblings
        pindf_pdf_obj *first = pindf_dict_getvalue2(&outlines_mod->obj->content.dict, "/First");
        pindf_pdf_obj *last = pindf_dict_getvalue2(&outlines_mod->obj->content.dict, "/Last");
        if (last != NULL && last->obj_type == PINDF_PDF_REF) {
            pindf_pdf_ind_obj *last_ind = NULL;
            pindf_pdf_obj *last_dict = resolve_ref(doc, parser, lexer, last, &last_ind);
            if (last_dict != NULL && last_dict->obj_type == PINDF_PDF_DICT && last_ind != NULL) {
                pindf_pdf_ind_obj *last_mod = make_ind_obj(last_ind->obj_num);
                last_mod->generation_num = last_ind->generation_num;
                last_mod->obj = pindf_pdf_obj_deepcopy(last_dict);
                pindf_dict_set_value2(&last_mod->obj->content.dict, "/Next",
                    make_ref_obj(outline_item->obj_num, outline_item->generation_num));
                pindf_modif_addentry(doc->modif, last_mod, last_mod->obj_num);

                pindf_dict_set_value2(&outline_item->obj->content.dict, "/Prev",
                    make_ref_obj(last_ind->obj_num, last_ind->generation_num));
            }
        }

        if (first == NULL) {
            pindf_dict_set_value2(&outlines_mod->obj->content.dict, "/First",
                make_ref_obj(outline_item->obj_num, outline_item->generation_num));
        }
        pindf_dict_set_value2(&outlines_mod->obj->content.dict, "/Last",
            make_ref_obj(outline_item->obj_num, outline_item->generation_num));

        pindf_pdf_obj *count = pindf_dict_getvalue2(&outlines_mod->obj->content.dict, "/Count");
        if (count != NULL && count->obj_type == PINDF_PDF_INT) {
            if (count->content.num < 0) {
                count->content.num -= 1;
            } else {
                count->content.num += 1;
            }
        } else {
            pindf_pdf_obj *new_count = pindf_pdf_obj_new(PINDF_PDF_INT);
            new_count->content.num = 1;
            pindf_dict_set_value2(&outlines_mod->obj->content.dict, "/Count", new_count);
        }

        pindf_modif_addentry(doc->modif, outline_item, outline_item->obj_num);
        pindf_modif_addentry(doc->modif, outlines_mod, outlines_mod->obj_num);
        printf("  Added Outline item (obj %d)\n", outline_item->obj_num);
    }

    // Save modifications to a output file
    FILE *output = fopen(argv[2], "wb");
    if (output == NULL) {
        perror("Failed to open output file");
        return 1;
    }

    // copy from input file to output file
    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("Failed to open input file");
        return 1;
    }
    fseek(input, 0, SEEK_END);
    long input_size = ftell(input);
    fseek(input, 0, SEEK_SET);
    for (size_t i = 0; i < input_size; i++) {
        int c = fgetc(input);
        fputc(c, output);
    }

    printf("\nSaving modifications to: %s\n", argv[2]);
    pindf_doc_save_modif(doc, output, true);
    fclose(output);
    fclose(input);

    // Verify output file was created
    FILE *check = fopen(argv[2], "rb");
    if (check == NULL) {
        perror("Failed to open output file for verification");
        return 1;
    }

    fseek(check, 0, SEEK_END);
    long modif_size = ftell(check) - input_size;
    fseek(check, 0, SEEK_SET);

    // Read and display the appended content
    printf("\n=== Appended content (modification) ===\n");
    char *buffer = (char *)malloc(modif_size + 1);
    fseek(check, input_size, SEEK_SET);
    size_t bytes_read = fread(buffer, 1, modif_size, check);
    buffer[bytes_read] = '\0';

    printf("%s", buffer);
    free(buffer);

    fclose(check);

    printf("modification size: %ld bytes\n", modif_size);
    assert(modif_size > 0); // Output should not be empty

    // Cleanup
    pindf_parser_clear(parser);
    pindf_lexer_clear(lexer);

    printf("\nTest completed successfully!\n");
    printf("Modified PDF saved as: %s\n", argv[2]);

    return 0;
}
