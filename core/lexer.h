/*
 Copyright 2026 fpg2012 (aka nth233)

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#pragma once
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "../type.h"
#include "../container/uchar_str.h"

#define PINDF_LEXER_BUFSIZE 1048576

/// @brief state of lexer
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

/// @brief event emitted by the lexer
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

/// @brief type of "regular" token
enum pindf_lexer_regtype {
	PINDF_LEXER_REGTYPE_UNK = 0,
	PINDF_LEXER_REGTYPE_NORM, /// NORM type should not exist in real pdf file
	PINDF_LEXER_REGTYPE_KWD,
	PINDF_LEXER_REGTYPE_INT = 1000,
	PINDF_LEXER_REGTYPE_REAL = 1001,
};

/// @brief keywoard enum
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

/// @brief option for event ignoration
enum pindf_lexer_opt {
	PINDF_LEXER_OPT_IGNORE_WS = 1,
	PINDF_LEXER_OPT_IGNORE_EOL = 2,
	PINDF_LEXER_OPT_IGNORE_CMT = 4,
	PINDF_LEXER_OPT_IGNORE_NO_EMIT = 8,
};

extern const char *pindf_lexer_keyword_list[];

/// @brief lexer of pdf
typedef struct {
	enum pindf_lexer_state state;
	enum pindf_lexer_state prev_state;
	uchar buf[PINDF_LEXER_BUFSIZE];
	size_t buf_end;
	/// for parenthesis matching
	int string_level;

	/// offset in pdf file of current token
	uint64 token_offset;
	/// offset of current file cursor, use less for buffer lexing
	uint64 offset;
} pindf_lexer;

/// @brief token
typedef struct {
	enum pindf_lexer_event event;
	/// raw byte string copied from file, can be NULL for some token types
	pindf_uchar_str *raw_str;
	/// only meaningful if event=regular, should be 0 otherwise
	enum pindf_lexer_regtype reg_type;
	/// only meaningful if reg_type=kwd, should be 0 otherwise
	enum pindf_kwd kwd;
	uint64 offset;
} pindf_token;

/// @brief create a new lexer
pindf_lexer *pindf_lexer_new();

/// @brief init a new lexer
void pindf_lexer_init(pindf_lexer *lexer);

/// @brief reset lexer to initial state, and clean up everything inside
void pindf_lexer_clear(pindf_lexer *lexer);

/// @brief lex a file, return lexical events if any
pindf_token *pindf_lex(pindf_lexer *lexer, FILE *file);
/// @brief lex a file with ignorance to certain events
pindf_token *pindf_lex_options(pindf_lexer *lexer, FILE *file, uint options);

/// @brief lex a buffer
/// offset is useless for buffer lexing
pindf_token *pindf_lex_from_buffer(pindf_lexer *lexer, uchar *buffer, size_t buf_offset, size_t buf_len, int *chars_read);
/// @brief lex a buffer with ignorance to certain events
pindf_token *pindf_lex_from_buffer_options(pindf_lexer *lexer, uchar *buffer, size_t buf_offset, size_t buf_len, int *chars_read, uint options);

/// @brief read raw stream content from pdf file
/// should not be called directly by applications
pindf_uchar_str *pindf_lex_get_stream(FILE *file, size_t len);

pindf_token *pindf_token_new(int event, pindf_uchar_str *raw_str, uint64 offset);

void pindf_token_destroy(pindf_token *token);

/// @brief finer distinguish among different regular tokens (real number vs integer vs keywords)
void pindf_token_regular_lex(pindf_token *token);