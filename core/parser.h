#pragma once

#include <stdlib.h>
#include "../container/simple_vector.h"
#include "../container/uchar_str.h"
#include "lexer.h"
#include "../pdf/obj.h"
#include "../pdf/doc.h"

enum pindf_symbol_type {
	PINDF_SYMBOL_TERM,
	PINDF_SYMBOL_NONTERM
};

enum pindf_file_part {
	PINDF_FILE_PART_HEADER,
	PINDF_FILE_PART_BODY,
	PINDF_FILE_PART_XREF,
	PINDF_FILE_PART_TRAILER,
	PINDF_FILE_PART_S_XREF,
};

typedef struct {
	enum pindf_symbol_type type; // 0 - term, 1 - non_term
	uint64 offset;
	union {
		pindf_token *term;
		pindf_pdf_obj *non_term;
	} content;
} pindf_symbol;

typedef struct {
	pindf_vector *symbol_stack; // buffer, vector of token
	pindf_vector *reduce_pos_stack;

	int reduce_pos;
} pindf_parser;

pindf_parser *pindf_parser_new();
void pindf_parser_init(pindf_parser *parser);
int pindf_parser_add_token(pindf_parser *parser, pindf_token * token);
int pindf_parser_add_stream(pindf_parser *parser, pindf_uchar_str *stream, uint64 content_offset);
void pindf_parser_destroy(pindf_parser *parser);

pindf_symbol *pindf_symbol_new_term(pindf_token *token);
pindf_symbol *pindf_symbol_new_nonterm(pindf_pdf_obj *pdf_obj, uint64 offset);

void pindf_parser_clear(pindf_parser *parser);