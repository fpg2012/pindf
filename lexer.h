#pragma once

#include "type.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

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
#define PINDF_LEXER_STATE_ERR			-1

#define PINDF_LEXER_BUFSIZE 1000

#define PINDF_LEXER_NO_EMIT		0
#define PINDF_LEXER_EMIT_REGULAR 	1
#define PINDF_LEXER_EMIT_DELIM		2
#define PDINF_LEXER_EMIT_LTR_STR	3
#define PINDF_LEXER_EMIT_HEX_STR	4
#define PINDF_LEXER_EMIT_EOL		5
#define PINDF_LEXER_EMIT_WHITE_SPACE	10
#define PINDF_LEXER_EMIT_ERR		-2
#define PINDF_LEXER_EMIT_EOF		-1

struct pindf_lexer {
	int state;
	int prev_state;
	uchar buf[PINDF_LEXER_BUFSIZE];
	size_t buf_end;
};

struct pindf_lexer_event {
	int event;
	uchar *emit_str;
};


void pindf_init_lexer(struct pindf_lexer *lexer);
struct pindf_lexer_event pindf_lex(struct pindf_lexer *lexer, FILE *file);