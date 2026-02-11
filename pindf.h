#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "container/uchar_str.h"
#include "type.h"
#include "core/lexer.h"
#include "core/parser.h"
#include "pdf/obj.h"
#include "pdf/doc.h"
#include "core/serialize.h"
#include "stream/filter.h"

#define PINDF_STREAM_BUF_LEN 1048576

int pindf_file_parse(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, uint64 file_len, pindf_doc **ret_doc);
int pindf_parse_one_obj(pindf_parser *parser, pindf_lexer *lexer, FILE *f, pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type);
int pindf_parse_one_obj_from_buffer(pindf_parser *parser, pindf_lexer *lexer, pindf_uchar_str *buffer, size_t offset, pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type);
int pindf_parse_xref(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, pindf_doc *doc, uint64 startxref);
uint64 pindf_find_startxref(FILE *fp, uint64 file_len_ptr);

pindf_pdf_obj *pindf_doc_getobj(pindf_doc *doc, pindf_parser *parser, pindf_lexer *lexer, uint64 obj_num);

int pindf_stream_decode(pindf_pdf_obj *stream, pindf_uchar_str *decoded);
int pindf_doc_save_modif(pindf_doc *doc, FILE *fp, bool compress_xref);