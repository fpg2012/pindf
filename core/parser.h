#pragma once

#include <stdlib.h>
#include "../container/simple_vector.h"
#include "../container/uchar_str.h"
#include "lexer.h"
#include "../pdf/obj.h"
#include "../pdf/doc.h"

#define PINDF_SYMBOL_TERM	0
#define PINDF_SYMBOL_NONTERM	1

#define PINDF_FILE_PART_HEADER	0
#define PINDF_FILE_PART_BODY	1
#define PINDF_FILE_PART_XREF	2
#define PINDF_FILE_PART_TRAILER	3
#define PINDF_FILE_PART_S_XREF	4

struct pindf_symbol {
	int type; // 0 - term, 1 - non_term
	uint64 offset;
	union {
		struct pindf_token *term;
		struct pindf_pdf_obj *non_term;
	} content;
};

struct pindf_parser {
	struct pindf_vector *symbol_stack; // buffer, vector of token
	struct pindf_vector *reduce_pos_stack;

	int reduce_pos;
};

struct pindf_parser *pindf_parser_new();
void pindf_parser_init(struct pindf_parser *parser);
int pindf_parser_add_token(struct pindf_parser *parser, struct pindf_token * token);
int pindf_parser_add_stream(struct pindf_parser *parser, struct pindf_uchar_str *stream);
void pindf_parser_destroy(void *parser);

struct pindf_symbol *pindf_symbol_new_term(struct pindf_token *token);
struct pindf_symbol *pindf_symbol_new_nonterm(struct pindf_pdf_obj *pdf_obj, uint64 offset);