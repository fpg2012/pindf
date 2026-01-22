#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "type.h"
#include "core/lexer.h"
#include "core/parser.h"
#include "pdf/obj.h"
#include "pdf/doc.h"

int pindf_file_parse(struct pindf_parser *parser, FILE *fp, uint64 file_len, struct pindf_doc **ret_doc);
int pindf_parse_one_obj(struct pindf_parser *parser, struct pindf_lexer *lexer, FILE *f, struct pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type);
int pindf_parse_xref(struct pindf_parser *parser, struct pindf_lexer *lexer, FILE *fp, struct pindf_doc *doc);
uint64 pindf_find_startxref(FILE *fp, uint64 file_len_ptr);
