#include "lexer.h"
#include "../logger/logger.h"

const char *pindf_lexer_keyword_list[] = {
	"",
	"endobj",
	"endstream",
	"f",
	"false",
	"n",
	"null",
	"obj",
	"R",
	"startxref",
	"stream",
	"trailer",
	"true",
	"xref",
};

static int _prev_state_emit[] = {
	[PINDF_LEXER_STATE_REGULAR] = PINDF_LEXER_EMIT_REGULAR,
	[PINDF_LEXER_STATE_WHITE_SPACE] = PINDF_LEXER_EMIT_WHITE_SPACE,
	[PINDF_LEXER_STATE_DELIM] = PINDF_LEXER_EMIT_DELIM,
	[PINDF_LEXER_STATE_IN_HEX_STR] = PINDF_LEXER_EMIT_HEX_STR,
	[PINDF_LEXER_STATE_IN_EOL] = PINDF_LEXER_EMIT_WHITE_SPACE,
	[PINDF_LEXER_STATE_OUT_EOL] = PINDF_LEXER_EMIT_EOL,
	[PINDF_LEXER_STATE_IN_NAME] = PINDF_LEXER_EMIT_NAME,
	[PINDF_LEXER_STATE_IN_COMMENT] = PINDF_LEXER_EMIT_COMMENT,

	[PINDF_LEXER_STATE_DEFAULT] = PINDF_LEXER_NO_EMIT,
	[PINDF_LEXER_STATE_IN_LTR_STR_ESC] = PINDF_LEXER_NO_EMIT,
	[PINDF_LEXER_STATE_OUT_STREAM] = PINDF_LEXER_NO_EMIT,

	[PINDF_LEXER_STATE_IN_STREAM] = PINDF_LEXER_EMIT_ERR,
	[PINDF_LEXER_STATE_IN_LTR_STR] = PINDF_LEXER_EMIT_ERR,
};


void _append_ch(pindf_lexer *lexer, char ch)
{
	if (lexer->buf_end == PINDF_LEXER_BUFSIZE) {
		PINDF_ERR("lexer buffer overflow!");
		PINDF_ERR("current state: %d", lexer->state);
		exit(-1);
	}
	if (lexer->buf_end == 0) {
		lexer->token_offset = lexer->offset;
	}
	lexer->buf[lexer->buf_end++] = ch;
}

int _last_char(pindf_lexer *lexer, uchar *ch)
{
	if (lexer->buf_end == 0) {
		return -1;
	}
	*ch = lexer->buf[lexer->buf_end-1];
	return 0;
}

pindf_uchar_str *_emit(pindf_lexer *lexer)
{
	pindf_uchar_str *emit_str = pindf_uchar_str_new();
	pindf_uchar_str_init(emit_str, lexer->buf_end);

	memcpy(emit_str->p, lexer->buf, lexer->buf_end);
	
	memset(lexer->buf, 0x00, lexer->buf_end);
	lexer->buf_end = 0;

	return emit_str;
}

pindf_lexer *pindf_lexer_new() {
	pindf_lexer *lexer = (pindf_lexer*)malloc(sizeof(pindf_lexer));
	pindf_lexer_init(lexer);

	return lexer;
}

void pindf_lexer_init(pindf_lexer *lexer)
{
	lexer->state = PINDF_LEXER_STATE_DEFAULT;
	memset(lexer->buf, 0x00, PINDF_LEXER_BUFSIZE);
	lexer->buf_end = 0;
	lexer->string_level = 0;
}

void pindf_lexer_clear(pindf_lexer *lexer)
{
	pindf_lexer_init(lexer);
}

pindf_uchar_str *pindf_lex_get_stream(FILE *file, size_t len)
{
	pindf_uchar_str *str = pindf_uchar_str_new();
	pindf_uchar_str_init(str, len);

	fread(str->p, len, 1, file);

	return str;
}

int _update_state(pindf_lexer *lexer, uchar ch, int *emit, pindf_uchar_str **emit_str)
{
	if (lexer->state == PINDF_LEXER_STATE_IN_LTR_STR) {
		switch (ch) {
		case '\\':
			_append_ch(lexer, ch);
			lexer->state = PINDF_LEXER_STATE_IN_LTR_STR_ESC;
			break;
		case ')':
			lexer->string_level--;
			if (lexer->string_level == 0) {
				*emit = PINDF_LEXER_EMIT_LTR_STR;
				*emit_str = _emit(lexer);
				lexer->state = PINDF_LEXER_STATE_DEFAULT;
			} else {
				_append_ch(lexer, ch);
			}
			break;
		case '(':
			lexer->string_level++;
		default:
			_append_ch(lexer, ch);
		}
	} else if (lexer->state == PINDF_LEXER_STATE_IN_LTR_STR_ESC) {
		_append_ch(lexer, ch);
		lexer->state = PINDF_LEXER_STATE_IN_LTR_STR;
	} else if (lexer->state == PINDF_LEXER_STATE_IN_HEX_STR) {
		if (ch == '<') {
			_append_ch(lexer, '<');
			_append_ch(lexer, '<');
			*emit = PINDF_LEXER_EMIT_DELIM;
			*emit_str = _emit(lexer);
			
			lexer->state = PINDF_LEXER_STATE_DEFAULT;
		} else if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
			_append_ch(lexer, ch);
		} else if (ch == '>') {
			*emit = PINDF_LEXER_EMIT_HEX_STR;
			*emit_str = _emit(lexer);
			lexer->state = PINDF_LEXER_STATE_DEFAULT;
		} else {
			*emit = PINDF_LEXER_EMIT_ERR;
		}
	} else if (lexer->state == PINDF_LEXER_STATE_IN_COMMENT) {
		switch (ch) {
			case '\n':
				*emit = PINDF_LEXER_EMIT_COMMENT;
				*emit_str = _emit(lexer);
				lexer->state = PINDF_LEXER_STATE_OUT_EOL;
				break;
			case '\r':
				*emit = PINDF_LEXER_EMIT_COMMENT;
				*emit_str = _emit(lexer);				
				lexer->state = PINDF_LEXER_STATE_IN_EOL;
				break;
			default:
				_append_ch(lexer, ch);
		}
	} else {
		switch (ch) {
		case '\0':
		case '\t':
		case 12:
		case ' ':
			switch (lexer->state) {
			case PINDF_LEXER_STATE_WHITE_SPACE:
				break;
			default:
				*emit = _prev_state_emit[lexer->prev_state];
				*emit_str = _emit(lexer);
				lexer->state = PINDF_LEXER_STATE_WHITE_SPACE;
			}
			break;
		case '\n':
			if (lexer->state == PINDF_LEXER_STATE_IN_EOL) {
				lexer->state = PINDF_LEXER_STATE_OUT_EOL;
			} else {
				*emit = _prev_state_emit[lexer->prev_state];
				*emit_str = _emit(lexer);
				lexer->state = PINDF_LEXER_STATE_OUT_EOL;
			}
			break;
		case '\r':
			*emit = _prev_state_emit[lexer->prev_state];
			*emit_str = _emit(lexer);				
			lexer->state = PINDF_LEXER_STATE_IN_EOL;
			break;
		case '(':
			lexer->string_level = 1;
			*emit = _prev_state_emit[lexer->prev_state];
			*emit_str = _emit(lexer);
			lexer->state = PINDF_LEXER_STATE_IN_LTR_STR;
			break;
		case ')':
			*emit = PINDF_LEXER_EMIT_ERR;
			break;
		case '<':
			*emit = _prev_state_emit[lexer->prev_state];
			*emit_str = _emit(lexer);
			lexer->state = PINDF_LEXER_STATE_IN_HEX_STR;
			break;
		case '>':
			;
			uchar last_char;
			if (lexer->state == PINDF_LEXER_STATE_DELIM && _last_char(lexer, &last_char) == 0 && last_char == '>') {
				_append_ch(lexer, ch);
				*emit = PINDF_LEXER_EMIT_DELIM;
				*emit_str = _emit(lexer);
				lexer->state = PINDF_LEXER_STATE_DEFAULT;
			} else {
				*emit = _prev_state_emit[lexer->prev_state];
				*emit_str = _emit(lexer);
				
				lexer->state = PINDF_LEXER_STATE_DELIM;
				_append_ch(lexer, ch);
			}
			break;
		case '[':
		case ']':
			*emit = _prev_state_emit[lexer->prev_state];
			*emit_str = _emit(lexer);

			_append_ch(lexer, ch);
			lexer->state = PINDF_LEXER_STATE_DELIM;
			break;
		case '/':
			*emit = _prev_state_emit[lexer->prev_state];
			*emit_str = _emit(lexer);

			_append_ch(lexer, ch);
			lexer->state = PINDF_LEXER_STATE_IN_NAME;
			break;
		case '%':
			*emit = _prev_state_emit[lexer->prev_state];
			*emit_str = _emit(lexer);

			lexer->state = PINDF_LEXER_STATE_IN_COMMENT;
			break;
		default:
			if (lexer->state == PINDF_LEXER_STATE_REGULAR) {
				_append_ch(lexer, ch);
			} else if (lexer->state == PINDF_LEXER_STATE_IN_NAME) {
				_append_ch(lexer, ch);
			} else {
				*emit = _prev_state_emit[lexer->prev_state];
				*emit_str = _emit(lexer);

				lexer->state = PINDF_LEXER_STATE_REGULAR;
				_append_ch(lexer, ch);
			}
		}
	}
	return 0;
}

pindf_token *pindf_lex(pindf_lexer *lexer, FILE *file)
{
	uchar ch;
	int emit = 0;
	pindf_uchar_str *emit_str = NULL;

	while (!emit) {
		lexer->prev_state = lexer->state;
		if (lexer->state == PINDF_LEXER_STATE_OUT_EOL) {
			emit = PINDF_LEXER_EMIT_EOL;
			lexer->state = PINDF_LEXER_STATE_DEFAULT;
			break;
		}

		int status = fgetc(file);
		++lexer->offset;
		if (status < 0) {
			if (feof(file)) {
				emit = PINDF_LEXER_EMIT_EOF;
				break;
			}
			if (ferror(file)) {
				emit = PINDF_LEXER_EMIT_ERR;
				perror("Error occurred while reading file");
				break;
			}
		}
		ch = (uchar)status;
		
		int ret = _update_state(lexer, ch, &emit, &emit_str);
		if (ret < 0) {
			emit = PINDF_LEXER_EMIT_ERR;
			break;
		}
	}

	pindf_token *token = pindf_token_new(emit, emit_str, lexer->token_offset);
	if (emit == PINDF_LEXER_EMIT_REGULAR)
		pindf_token_regular_lex(token);
	return token;
}

pindf_token *pindf_token_new(int event, pindf_uchar_str *raw_str, uint64 offset)
{
	pindf_token *token = (pindf_token*)malloc(sizeof(pindf_token));
	token->event = event;
	token->raw_str = raw_str;
	token->offset = offset;
	token->kwd = 0;
	token->reg_type = 0;
	return token;
}

void pindf_token_regular_lex(pindf_token *token) {
	assert(token->event == PINDF_LEXER_EMIT_REGULAR && "[regular lex] not regular token to parse");
	assert(token->raw_str && "[regular lex] raw_str is NULL");

	uchar *beg = token->raw_str->p;
	uchar *end = token->raw_str->p + token->raw_str->len;

	enum pindf_lexer_regtype reg_type = PINDF_LEXER_REGTYPE_NORM;
	enum pindf_kwd kwd_type = PINDF_KWD_UNK;

	if (beg == end) {
		return;
	}

	// test keyword
	for (int i = 1; i < PINDF_KWD_END; i++) {
		const char *kwd = pindf_lexer_keyword_list[i];
		size_t kwd_len = strlen(kwd);
		if (kwd_len != token->raw_str->len)
			continue;
		if (memcmp(beg, pindf_lexer_keyword_list[i], kwd_len) == 0) {
			reg_type = PINDF_LEXER_REGTYPE_KWD;
			kwd_type = i;
			break;
		}
	}
	if (reg_type != PINDF_LEXER_REGTYPE_NORM) {
		token->reg_type = reg_type;
		token->kwd = kwd_type;
		return;
	}

	// test int
	uchar *p = beg;
	int int_state = 0;
	reg_type = PINDF_LEXER_REGTYPE_INT;
	while (p != end) {
		if (int_state == 0) {
			if (*p >= '0' && *p <= '9') {
				int_state = 1;
			} else if (*p == '+' || *p == '-') {
				;
			} else {
				reg_type = PINDF_LEXER_REGTYPE_NORM;
				break;
			}
		} else if (int_state == 1) {
			if (!(*p >= '0' && *p <= '9')) {
				reg_type = PINDF_LEXER_REGTYPE_NORM;
				break;
			}
		} else {
			perror("Failed to parse int. This should not happen.");
		}
		p++;
	}
	
	if (reg_type != PINDF_LEXER_REGTYPE_NORM) {
		token->reg_type = reg_type;
		return;
	}
	
	// test real
	p = beg;
	int real_state = 0;
	reg_type = PINDF_LEXER_REGTYPE_REAL;
	while (p != end) {
		if (real_state == 0) {
			if (*p == '+' || *p == '-') {
				;
			} else if (*p == '0') {
				real_state = 2;
			} else if (*p >= '1' && *p <= '9') {
				real_state = 1;
			} else {
				reg_type = PINDF_LEXER_REGTYPE_NORM;
				break;
			}
		} else if (real_state == 1) {
			if (*p >= '0' && *p <= '9') {
				;
			} else if (*p == '.') {
				real_state = 3;
			} else {
				reg_type = PINDF_LEXER_REGTYPE_NORM;
				break;
			}
		} else if (real_state == 2) {
			if (*p != '.') {
				reg_type = PINDF_LEXER_REGTYPE_NORM;
				break;
			} else {
				real_state = 3;
			}
		} else if (real_state == 3) {
			if (!(*p >= '0' && *p <= '9')) {
				reg_type = PINDF_LEXER_REGTYPE_NORM;
				break;
			}
		} else {
			perror("Failed to parse real. This should not happen.");
		}

		p++;
	}

	if (reg_type != PINDF_LEXER_REGTYPE_NORM) {
		token->reg_type = reg_type;
		return;
	}

	// fall back to norm
	token->reg_type = reg_type;
}

pindf_token *pindf_lex_options(pindf_lexer *lexer, FILE *file, uint options)
{
	pindf_token *token;

	uint ignore = 0;
	do {
		ignore = 0;
		token = pindf_lex(lexer, file);

		ignore |= token->event == PINDF_LEXER_EMIT_WHITE_SPACE && (options & PINDF_LEXER_OPT_IGNORE_WS);
		ignore |= token->event == PINDF_LEXER_EMIT_COMMENT && (options & PINDF_LEXER_OPT_IGNORE_CMT);
		ignore |= token->event == PINDF_LEXER_EMIT_EOL && (options & PINDF_LEXER_OPT_IGNORE_EOL);
		ignore |= token->event == PINDF_LEXER_NO_EMIT && (options & PINDF_LEXER_NO_EMIT);
	} while (ignore);
	return token;
}

pindf_token *pindf_lex_from_buffer(pindf_lexer *lexer, uchar *buffer, size_t buf_offset, size_t buf_len, int *chars_read)
{
	uchar *beg = buffer + buf_offset, *p = beg;

	uchar ch;
	int emit = 0;
	pindf_uchar_str *emit_str = NULL;

	while (!emit) {
		lexer->prev_state = lexer->state;
		if (lexer->state == PINDF_LEXER_STATE_OUT_EOL) {
			emit = PINDF_LEXER_EMIT_EOL;
			lexer->state = PINDF_LEXER_STATE_DEFAULT;
			break;
		}

		if (p == buffer + buf_len) {
			emit = PINDF_LEXER_EMIT_EOF;
			break;
		}
		int status = *(p++);
		++lexer->offset;
		ch = (uchar)status;
		
		int ret = _update_state(lexer, ch, &emit, &emit_str);
		if (ret < 0) {
			emit = PINDF_LEXER_EMIT_ERR;
			break;
		}
	}

	pindf_token *token = pindf_token_new(emit, emit_str, lexer->token_offset);
	if (emit == PINDF_LEXER_EMIT_REGULAR)
		pindf_token_regular_lex(token);
	*chars_read = p - beg;
	return token;
}

pindf_token *pindf_lex_from_buffer_options(
	pindf_lexer *lexer,
	uchar *buffer,
	size_t buf_offset,
	size_t buf_len,
	int *chars_read,
	uint options)
{
	pindf_token *token;
	int chars_read_inner = 0;
	size_t cursor = 0;

	uint ignore = 0;
	do {
		ignore = 0;
		token = pindf_lex_from_buffer(lexer, buffer, buf_offset + cursor, buf_len, &chars_read_inner);
		cursor += chars_read_inner;

		ignore |= token->event == PINDF_LEXER_EMIT_WHITE_SPACE && (options & PINDF_LEXER_OPT_IGNORE_WS);
		ignore |= token->event == PINDF_LEXER_EMIT_COMMENT && (options & PINDF_LEXER_OPT_IGNORE_CMT);
		ignore |= token->event == PINDF_LEXER_EMIT_EOL && (options & PINDF_LEXER_OPT_IGNORE_EOL);
		ignore |= token->event == PINDF_LEXER_NO_EMIT && (options & PINDF_LEXER_NO_EMIT);
		if (ignore) {
			pindf_token_destroy(token);
			free(token);
		}
	} while (ignore && cursor < buf_len);

	*chars_read = (int)cursor;
	return token;
}

void pindf_token_destroy(pindf_token *token)
{
	if (token->raw_str)
		pindf_uchar_str_destroy(token->raw_str);
	free(token->raw_str);
}