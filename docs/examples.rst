Examples
========

This page provides practical examples of using the pindf library.

Basic PDF Parsing
-----------------

The simplest way to parse a PDF file:

.. code-block:: c

    #include "pindf.h"
    #include <stdio.h>
    #include <stdlib.h>

    int main(int argc, char **argv) {
        if (argc != 2) {
            fprintf(stderr, "Usage: %s <pdf_file>\n", argv[0]);
            return 1;
        }

        // Set log level (optional)
        pindf_set_log_level(PINDF_LOG_LEVEL_INFO);

        // Create lexer and parser instances
        pindf_lexer *lexer = pindf_lexer_new();
        pindf_parser *parser = pindf_parser_new();

        FILE *f = fopen(argv[1], "rb");
        if (!f) {
            perror("Failed to open file");
            return 1;
        }

        // Get file size
        fseek(f, 0, SEEK_END);
        uint64 file_len = ftell(f);
        fseek(f, 0, SEEK_SET);

        // Parse the PDF
        pindf_doc *doc = NULL;
        int ret = pindf_file_parse(parser, lexer, f, file_len, &doc);
        if (ret < 0) {
            fprintf(stderr, "Failed to parse PDF\n");
            fclose(f);
            return 1;
        }

        printf("Successfully parsed PDF\n");
        printf("PDF version: %s\n", doc->pdf_version);
        printf("Xref offset: %d\n", doc->xref_offset);
        printf("Trailer entries: %zu\n", doc->trailer.keys->len);

        // Cleanup
        pindf_parser_clear(parser);
        pindf_lexer_clear(lexer);
        fclose(f);

        return 0;
    }

Accessing PDF Objects
---------------------

Retrieve and examine specific PDF objects:

.. code-block:: c

    // Continue from previous example after parsing
    // Get object by number
    pindf_pdf_obj *obj = pindf_doc_getobj(doc, parser, lexer, 4);
    if (obj) {
        printf("Object 4 found\n");

        if (obj->obj_type == PINDF_PDF_DICT) {
            printf("Object is a dictionary\n");

            // Get specific value from dictionary
            pindf_pdf_obj *type_val = pindf_dict_getvalue2(&obj->content.dict, "/Type");
            if (type_val && type_val->obj_type == PINDF_PDF_NAME) {
                printf("Type: %.*s\n",
                       (int)type_val->content.name.len,
                       type_val->content.name.p);
            }
        }
    }

    // List all objects in document
    printf("\n=== Document objects ===\n");
    for (int i = 0; i < doc->xref->size; ++i) {
        pindf_pdf_obj *obj = pindf_doc_getobj(doc, parser, lexer, i);
        if (obj) {
            printf("Object %d: type %d\n", i, obj->obj_type);
        }
    }

Modifying PDF Files
-------------------

Add an outline (bookmark) to a PDF:

.. code-block:: c

    #include "pindf.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <assert.h>

    int main(int argc, const char **argv) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s <input_pdf> <output_pdf>\n", argv[0]);
            return 1;
        }

        pindf_set_log_level(PINDF_LOG_LEVEL_INFO);
        pindf_lexer *lexer = pindf_lexer_new();
        pindf_parser *parser = pindf_parser_new();

        FILE *f = fopen(argv[1], "rb");
        if (!f) {
            perror("Failed to open input file");
            return 1;
        }

        // Get file size
        fseek(f, 0, SEEK_END);
        uint64 file_len = ftell(f);
        fseek(f, 0, SEEK_SET);

        // Parse PDF
        pindf_doc *doc = NULL;
        int ret = pindf_file_parse(parser, lexer, f, file_len, &doc);
        if (ret < 0) {
            fprintf(stderr, "Failed to parse PDF\n");
            fclose(f);
            return 1;
        }

        printf("PDF parsed successfully\n");

        // Create modifications structure if needed
        if (doc->modif == NULL) {
            doc->modif = pindf_modif_new(doc->xref->size);
        }

        // Find root catalog
        pindf_pdf_obj *root_ref = pindf_dict_getvalue2(&doc->trailer, "/Root");
        pindf_pdf_ind_obj *root_ind = NULL;
        pindf_pdf_obj *root_dict = resolve_ref(doc, parser, lexer, root_ref, &root_ind);

        if (!root_dict || root_dict->obj_type != PINDF_PDF_DICT) {
            fprintf(stderr, "Could not find root catalog\n");
            return 1;
        }

        // Create a new outline item
        int new_obj_num = doc->modif->max_obj_num + 1;
        pindf_pdf_ind_obj *outline_item = malloc(sizeof(pindf_pdf_ind_obj));
        *outline_item = (pindf_pdf_ind_obj){
            .obj_num = new_obj_num,
            .generation_num = 0,
            .obj = NULL,
            .start_pos = 0,
        };

        // Create outline dictionary
        outline_item->obj = pindf_pdf_obj_new(PINDF_PDF_DICT);
        outline_item->obj->content.dict.keys = pindf_vector_new(8, sizeof(pindf_pdf_obj*));
        outline_item->obj->content.dict.values = pindf_vector_new(8, sizeof(pindf_pdf_obj*));

        // Set title
        pindf_pdf_obj *title = pindf_pdf_obj_new(PINDF_PDF_LTR_STR);
        title->content.ltr_str = pindf_uchar_str_from_cstr("New Bookmark", 12);
        pindf_dict_set_value2(&outline_item->obj->content.dict, "/Title", title);

        // Add to modifications
        pindf_modif_addentry(doc->modif, outline_item, outline_item->obj_num);

        // Save modified document
        FILE *output = fopen(argv[2], "wb");
        if (!output) {
            perror("Failed to open output file");
            return 1;
        }

        // Copy original content
        fseek(f, 0, SEEK_SET);
        for (uint64 i = 0; i < file_len; i++) {
            int c = fgetc(f);
            fputc(c, output);
        }

        // Save modifications
        pindf_doc_save_modif(doc, output, true);

        printf("Modified PDF saved to: %s\n", argv[2]);

        // Cleanup
        fclose(f);
        fclose(output);
        pindf_parser_clear(parser);
        pindf_lexer_clear(lexer);

        return 0;
    }

Stream Decoding
---------------

Decode compressed stream objects:

.. code-block:: c

    // Assuming 'stream_obj' is a PINDF_PDF_STREAM object
    if (stream_obj->obj_type == PINDF_PDF_STREAM) {
        pindf_uchar_str decoded;
        pindf_uchar_str_init(&decoded, 1024);

        int ret = pindf_stream_decode(stream_obj, &decoded);
        if (ret == 0) {
            printf("Decoded stream (%zu bytes):\n", decoded.len);
            // Use decoded.p for processing
        }

        pindf_uchar_str_free(&decoded);
    }

Working with PDF Dictionaries
-----------------------------

Create and manipulate PDF dictionaries:

.. code-block:: c

    // Create a new dictionary
    pindf_pdf_obj *dict = pindf_pdf_obj_new(PINDF_PDF_DICT);
    dict->content.dict.keys = pindf_vector_new(8, sizeof(pindf_pdf_obj*));
    dict->content.dict.values = pindf_vector_new(8, sizeof(pindf_pdf_obj*));

    // Add integer value
    pindf_pdf_obj *int_val = pindf_pdf_obj_new(PINDF_PDF_INT);
    int_val->content.num = 42;
    pindf_dict_set_value2(&dict->content.dict, "/SomeInteger", int_val);

    // Add string value
    pindf_pdf_obj *str_val = pindf_pdf_obj_new(PINDF_PDF_LTR_STR);
    str_val->content.ltr_str = pindf_uchar_str_from_cstr("Hello PDF", 9);
    pindf_dict_set_value2(&dict->content.dict, "/Title", str_val);

    // Add array
    pindf_pdf_obj *arr = pindf_pdf_obj_new(PINDF_PDF_ARRAY);
    arr->content.array = pindf_vector_new(4, sizeof(pindf_pdf_obj*));

    pindf_pdf_obj *item1 = pindf_pdf_obj_new(PINDF_PDF_INT);
    item1->content.num = 1;
    pindf_vector_append(arr->content.array, &item1);

    pindf_pdf_obj *item2 = pindf_pdf_obj_new(PINDF_PDF_NAME);
    item2->content.name = pindf_uchar_str_from_cstr("/Item", 5);
    pindf_vector_append(arr->content.array, &item2);

    pindf_dict_set_value2(&dict->content.dict, "/MyArray", arr);

    // Retrieve value
    pindf_pdf_obj *title = pindf_dict_getvalue2(&dict->content.dict, "/Title");
    if (title && title->obj_type == PINDF_PDF_LTR_STR) {
        printf("Title: %.*s\n",
               (int)title->content.ltr_str.len,
               title->content.ltr_str.p);
    }

Common PDF Object Types
-----------------------

pindf supports all standard PDF object types:

+-------------------+------------------------+------------------------------------------+
| Type              | Enum constant          | Description                              |
+===================+========================+==========================================+
| Integer           | ``PINDF_PDF_INT``      | Integer numbers                          |
+-------------------+------------------------+------------------------------------------+
| Real              | ``PINDF_PDF_REAL``     | Floating point numbers                   |
+-------------------+------------------------+------------------------------------------+
| Dictionary        | ``PINDF_PDF_DICT``     | Key-value pairs with name keys           |
+-------------------+------------------------+------------------------------------------+
| Array             | ``PINDF_PDF_ARRAY``    | Ordered list of objects                  |
+-------------------+------------------------+------------------------------------------+
| Literal String    | ``PINDF_PDF_LTR_STR``  | Strings in parentheses                   |
+-------------------+------------------------+------------------------------------------+
| Hexadecimal String| ``PINDF_PDF_HEX_STR``  | Strings in angle brackets                |
+-------------------+------------------------+------------------------------------------+
| Boolean           | ``PINDF_PDF_BOOL``     | ``true`` or ``false``                    |
+-------------------+------------------------+------------------------------------------+
| Stream            | ``PINDF_PDF_STREAM``   | Dictionary + binary data                 |
+-------------------+------------------------+------------------------------------------+
| Name              | ``PINDF_PDF_NAME``     | Names starting with ``/``                |
+-------------------+------------------------+------------------------------------------+
| Null              | ``PINDF_PDF_NULL``     | Null object                              |
+-------------------+------------------------+------------------------------------------+
| Reference         | ``PINDF_PDF_REF``      | Object reference ``n g R``               |
+-------------------+------------------------+------------------------------------------+
| Indirect Object   | ``PINDF_PDF_IND_OBJ``  | Object with generation number            |
+-------------------+------------------------+------------------------------------------+

Error Handling
--------------

Most pindf functions return an integer error code:

- ``0``: Success
- Negative values: Error codes (see ``pindf.h`` for definitions)
- ``1``: Special case for ``pindf_parse_one_obj`` when object matches target type

Example error handling:

.. code-block:: c

    int ret = pindf_file_parse(parser, lexer, f, file_len, &doc);
    if (ret < 0) {
        fprintf(stderr, "Parse failed with error: %d\n", ret);
        // Handle error
    }

Memory Management
-----------------

pindf uses manual memory management. Follow these guidelines:

1. **Create objects** with ``pindf_pdf_obj_new(type)``
2. **Free objects** when done (not always required for retrieved objects)
3. **Clear lexer/parser** with ``pindf_lexer_clear()`` and ``pindf_parser_clear()``
4. **Close files** after use

See also the API reference for specific cleanup functions.