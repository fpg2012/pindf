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

/// @brief Unified struct for both obejcts (non-terminals) and tokens (terminals)
typedef struct {
	/// 0 - term, 1 - non_term
	enum pindf_symbol_type type;
	uint64 offset;
	union {
		pindf_token *term;
		pindf_pdf_obj *non_term;
	} content;
} pindf_symbol;

/// @brief Simple reduction based parser. (PDA, Push-down automaton)
typedef struct {
	pindf_vector *symbol_stack; /// buffer, vector of token
	pindf_vector *reduce_pos_stack; /// stack of position for reduction

	int reduce_pos; /// start pos of the current object in symbol stack
} pindf_parser;

/// @brief creata new parser on heap
/// @return an initialize parser referece
pindf_parser *pindf_parser_new();

/// @brief init a parser
void pindf_parser_init(pindf_parser *parser);

/// @brief reset parser
/// this function will cleanup everything in parser, and reset it to init state
void pindf_parser_clear(pindf_parser *parser);

/// @brief push token to the parser
/// on the ownership of token
/// For any ltrstr, hexstr or name, parser got the ownership of inner raw_str.
/// For any delim, parser got ownership of the whole token.
/// Otherwise, parser do not get the ownership of anything.
int pindf_parser_add_token(pindf_parser *parser, pindf_token *token);

int pindf_parser_add_stream(pindf_parser *parser, pindf_uchar_str *stream, uint64 content_offset);

/// @brief destroy the parser
/// This will free symbol in the stack. BUT will not free the pdf objects in symbols.
/// Because owner of pdf objects should be ind_obj table
void pindf_parser_destroy(pindf_parser *parser);

pindf_symbol *pindf_symbol_new_term(pindf_token *token);
pindf_symbol *pindf_symbol_new_nonterm(pindf_pdf_obj *pdf_obj, uint64 offset);