#pragma once
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "../type.h"
#include "../container/uchar_str.h"

#define PINDF_LEXER_BUFSIZE 1048576

enum pindf_lexer_state {
	PINDF_LEXER_STATE_DEFAULT = 0,
	PINDF_LEXER_STATE_REGULAR,
	PINDF_LEXER_STATE_WHITE_SPACE,
	PINDF_LEXER_STATE_DELIM,
	PINDF_LEXER_STATE_IN_LTR_STR,
	PINDF_LEXER_STATE_IN_STREAM,
	PINDF_LEXER_STATE_IN_LTR_STR_ESC,
	PINDF_LEXER_STATE_IN_EOL,
	PINDF_LEXER_STATE_IN_HEX_STR,
	PINDF_LEXER_STATE_OUT_STREAM,
	PINDF_LEXER_STATE_OUT_EOL,
	PINDF_LEXER_STATE_IN_COMMENT,
	PINDF_LEXER_STATE_IN_NAME,
	PINDF_LEXER_STATE_ERR = -1,
};

enum pindf_lexer_event {
	PINDF_LEXER_NO_EMIT = 0,
	PINDF_LEXER_EMIT_REGULAR,
	PINDF_LEXER_EMIT_DELIM,
	PINDF_LEXER_EMIT_LTR_STR,
	PINDF_LEXER_EMIT_HEX_STR,
	PINDF_LEXER_EMIT_EOL,
	PINDF_LEXER_EMIT_NAME,	
	PINDF_LEXER_EMIT_WHITE_SPACE = 10,
	PINDF_LEXER_EMIT_COMMENT,
	PINDF_LEXER_EMIT_ERR = -2,
	PINDF_LEXER_EMIT_EOF = -1,
};

enum pindf_lexer_regtype {
	PINDF_LEXER_REGTYPE_UNK = 0,
	PINDF_LEXER_REGTYPE_NORM,
	PINDF_LEXER_REGTYPE_KWD,
	PINDF_LEXER_REGTYPE_INT = 1000,
	PINDF_LEXER_REGTYPE_REAL = 1001,
};

enum pindf_kwd {
	PINDF_KWD_UNK = 0,
	PINDF_KWD_endobj,
	PINDF_KWD_endstream,
	PINDF_KWD_f,
	PINDF_KWD_false,
	PINDF_KWD_n,
	PINDF_KWD_null,
	PINDF_KWD_obj,
	PINDF_KWD_R,
	PINDF_KWD_startxref,
	PINDF_KWD_stream,
	PINDF_KWD_trailer,
	PINDF_KWD_true,
	PINDF_KWD_xref,
	PINDF_KWD_END,
};

enum pindf_lexer_opt {
	PINDF_LEXER_OPT_IGNORE_WS = 1,
	PINDF_LEXER_OPT_IGNORE_EOL = 2,
	PINDF_LEXER_OPT_IGNORE_CMT = 4,
	PINDF_LEXER_OPT_IGNORE_NO_EMIT = 8,
};

extern const char *pindf_lexer_keyword_list[];

typedef struct {
	enum pindf_lexer_state state;
	enum pindf_lexer_state prev_state;
	uchar buf[PINDF_LEXER_BUFSIZE];
	size_t buf_end;
	int string_level;

	uint64 token_offset;
	uint64 offset;
} pindf_lexer;

typedef struct {
	enum pindf_lexer_event event;
	pindf_uchar_str *raw_str;
	enum pindf_lexer_regtype reg_type; 	// only meaningful if event=regular
	enum pindf_kwd kwd; 	// only meaningful if reg_type=kwd
	uint64 offset;
} pindf_token;


pindf_lexer *pindf_lexer_new();
void pindf_lexer_init(pindf_lexer *lexer);
void pindf_lexer_clear(pindf_lexer *lexer);
pindf_token *pindf_lex(pindf_lexer *lexer, FILE *file);
pindf_token *pindf_lex_options(pindf_lexer *lexer, FILE *file, uint options);

pindf_token *pindf_lex_from_buffer(pindf_lexer *lexer, uchar *buffer, size_t buf_offset, size_t buf_len, int *chars_read);
pindf_token *pindf_lex_from_buffer_options(pindf_lexer *lexer, uchar *buffer, size_t buf_offset, size_t buf_len, int *chars_read, uint options);

pindf_uchar_str *pindf_lex_get_stream(FILE *file, size_t len);

pindf_token *pindf_token_new(int event, pindf_uchar_str *raw_str, uint64 offset);
void pindf_token_regular_lex(pindf_token *token);
void pindf_token_destroy(pindf_token *token);