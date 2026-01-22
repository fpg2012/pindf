#pragma once
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "../type.h"
#include "../container/uchar_str.h"

#define PINDF_LEXER_STATE_DEFAULT		0
#define PINDF_LEXER_STATE_REGULAR		1
#define PINDF_LEXER_STATE_WHITE_SPACE		2
#define PINDF_LEXER_STATE_DELIM			3
#define PINDF_LEXER_STATE_IN_LTR_STR		4
#define PINDF_LEXER_STATE_IN_STREAM		5
#define PINDF_LEXER_STATE_IN_LTR_STR_ESC	6
#define PINDF_LEXER_STATE_IN_EOL		7
#define PINDF_LEXER_STATE_IN_HEX_STR		8
#define PINDF_LEXER_STATE_OUT_STREAM		9
#define PINDF_LEXER_STATE_OUT_EOL		10
#define PINDF_LEXER_STATE_IN_COMMENT		11
#define PINDF_LEXER_STATE_IN_NAME		12
#define PINDF_LEXER_STATE_ERR			-1

#define PINDF_LEXER_BUFSIZE 32768

#define PINDF_LEXER_NO_EMIT			0
#define PINDF_LEXER_EMIT_REGULAR 		1
#define PINDF_LEXER_EMIT_DELIM			2
#define PINDF_LEXER_EMIT_LTR_STR		3
#define PINDF_LEXER_EMIT_HEX_STR		4
#define PINDF_LEXER_EMIT_EOL			5
#define PINDF_LEXER_EMIT_NAME			6
#define PINDF_LEXER_EMIT_WHITE_SPACE		10
#define PINDF_LEXER_EMIT_COMMENT		11
#define PINDF_LEXER_EMIT_ERR			-2
#define PINDF_LEXER_EMIT_EOF			-1

#define PINDF_LEXER_REGTYPE_NORM	0
#define PINDF_LEXER_REGTYPE_KWD		1
#define PINDF_LEXER_REGTYPE_INT		1000
#define PINDF_LEXER_REGTYPE_REAL	1001

#define PINDF_KWD_UNK		0
#define PINDF_KWD_endobj	1
#define PINDF_KWD_endstream	2
#define PINDF_KWD_f		3
#define PINDF_KWD_false		4
#define PINDF_KWD_n		5
#define PINDF_KWD_null		6
#define PINDF_KWD_obj		7
#define PINDF_KWD_R		8
#define PINDF_KWD_startxref	9
#define PINDF_KWD_stream	10
#define PINDF_KWD_trailer	11
#define PINDF_KWD_true		12
#define PINDF_KWD_xref		13
#define PINDF_KWD_END		14

#define PINDF_LEXER_OPT_IGNORE_WS	1
#define PINDF_LEXER_OPT_IGNORE_EOL	2
#define PINDF_LEXER_OPT_IGNORE_CMT	4
#define PINDF_LEXER_OPT_IGNORE_NO_EMIT	8

extern const char *pindf_lexer_keyword_list[];

struct pindf_lexer {
	int state;
	int prev_state;
	uchar buf[PINDF_LEXER_BUFSIZE];
	size_t buf_end;
	int string_level;

	uint64 token_offset;
	uint64 offset;
};

struct pindf_token {
	int event;
	struct pindf_uchar_str *raw_str;
	int reg_type; 	// only meaningful if event=regular
	int kwd; 	// only meaningful if reg_type=kwd
	uint64 offset;
};


struct pindf_lexer *pindf_lexer_new();
void pindf_lexer_init(struct pindf_lexer *lexer);
struct pindf_token *pindf_lex(struct pindf_lexer *lexer, FILE *file);
struct pindf_token *pindf_lex_options(struct pindf_lexer *lexer, FILE *file, uint options);
struct pindf_uchar_str *pindf_lex_get_stream(FILE *file, size_t len);

struct pindf_token *pindf_token_new(int event, struct pindf_uchar_str *raw_str, uint64 offset);
void pindf_token_regular_lex(struct pindf_token *token);