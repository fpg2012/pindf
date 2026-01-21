#include "lexer.h"
#include "uchar_str.h"

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


void _append_ch(struct pindf_lexer *lexer, char ch)
{
	if (lexer->buf_end == PINDF_LEXER_BUFSIZE) {
		perror("lexer buffer overflow!");
		fprintf(stderr, "current state: %d\n", lexer->state);
		exit(0);
	}
	if (lexer->buf_end == 0) {
		lexer->token_offset = lexer->offset;
	}
	lexer->buf[lexer->buf_end++] = ch;
}

int _last_char(struct pindf_lexer *lexer, uchar *ch)
{
	if (lexer->buf_end == 0) {
		return -1;
	}
	*ch = lexer->buf[lexer->buf_end-1];
	return 0;
}

struct pindf_uchar_str *_emit(struct pindf_lexer *lexer)
{
	struct pindf_uchar_str *emit_str = pindf_uchar_str_new();
	pindf_uchar_str_init(emit_str, lexer->buf_end);

	memcpy(emit_str->p, lexer->buf, lexer->buf_end);
	
	memset(lexer->buf, 0x00, lexer->buf_end);
	lexer->buf_end = 0;

	return emit_str;
}

struct pindf_lexer *pindf_lexer_new() {
	struct pindf_lexer *lexer = (struct pindf_lexer*)malloc(sizeof(struct pindf_lexer));
	pindf_lexer_init(lexer);

	return lexer;
}

void pindf_lexer_init(struct pindf_lexer *lexer)
{
	lexer->state = PINDF_LEXER_STATE_DEFAULT;
	memset(lexer->buf, 0x00, PINDF_LEXER_BUFSIZE);
	lexer->buf_end = 0;
	lexer->string_level = 0;
}

struct pindf_uchar_str *pindf_lex_get_stream(FILE *file, size_t len)
{
	struct pindf_uchar_str *str = pindf_uchar_str_new();
	pindf_uchar_str_init(str, len);

	fread(str->p, len, 1, file);

	return str;
}

struct pindf_token *pindf_lex(struct pindf_lexer *lexer, FILE *file)
{
	uchar ch;
	int emit = 0;
	struct pindf_uchar_str *emit_str = NULL;

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
		

		if (lexer->state == PINDF_LEXER_STATE_IN_LTR_STR) {
			switch (ch) {
			case '\\':
				_append_ch(lexer, ch);
				lexer->state = PINDF_LEXER_STATE_IN_LTR_STR_ESC;
				break;
			case ')':
				lexer->string_level--;
				if (lexer->string_level == 0) {
					emit = PINDF_LEXER_EMIT_LTR_STR;
					emit_str = _emit(lexer);
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
				emit = PINDF_LEXER_EMIT_DELIM;
				emit_str = _emit(lexer);
				
				lexer->state = PINDF_LEXER_STATE_DEFAULT;
			} else if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
				_append_ch(lexer, ch);
			} else if (ch == '>') {
				emit = PINDF_LEXER_EMIT_HEX_STR;
				emit_str = _emit(lexer);
				lexer->state = PINDF_LEXER_STATE_DEFAULT;
			} else {
				emit = PINDF_LEXER_EMIT_ERR;
			}
		} else if (lexer->state == PINDF_LEXER_STATE_IN_COMMENT) {
			switch (ch) {
				case '\n':
					emit = PINDF_LEXER_EMIT_COMMENT;
					emit_str = _emit(lexer);
					lexer->state = PINDF_LEXER_STATE_OUT_EOL;
					break;
				case '\r':
					emit = PINDF_LEXER_EMIT_COMMENT;
					emit_str = _emit(lexer);				
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
					emit = _prev_state_emit[lexer->prev_state];
					emit_str = _emit(lexer);
					lexer->state = PINDF_LEXER_STATE_WHITE_SPACE;
				}
				break;
			case '\n':
				if (lexer->state == PINDF_LEXER_STATE_IN_EOL) {
					lexer->state = PINDF_LEXER_STATE_OUT_EOL;
				} else {
					emit = _prev_state_emit[lexer->prev_state];
					emit_str = _emit(lexer);
					lexer->state = PINDF_LEXER_STATE_OUT_EOL;
				}
				break;
			case '\r':
				emit = _prev_state_emit[lexer->prev_state];
				emit_str = _emit(lexer);				
				lexer->state = PINDF_LEXER_STATE_IN_EOL;
				break;
			case '(':
				lexer->string_level = 1;
				emit = _prev_state_emit[lexer->prev_state];
				emit_str = _emit(lexer);
				lexer->state = PINDF_LEXER_STATE_IN_LTR_STR;
				break;
			case ')':
				emit = PINDF_LEXER_EMIT_ERR;
				break;
			case '<':
				emit = _prev_state_emit[lexer->prev_state];
				emit_str = _emit(lexer);
				lexer->state = PINDF_LEXER_STATE_IN_HEX_STR;
				break;
			case '>':
				if (lexer->state == PINDF_LEXER_STATE_DELIM) {
					uchar last_char;
					if (_last_char(lexer, &last_char) == 0 && last_char == '>') {
						_append_ch(lexer, ch);
						emit = PINDF_LEXER_EMIT_DELIM;
						emit_str = _emit(lexer);
						lexer->state = PINDF_LEXER_STATE_DEFAULT;
					}
				} else {
					emit = _prev_state_emit[lexer->prev_state];
					emit_str = _emit(lexer);
					
					lexer->state = PINDF_LEXER_STATE_DELIM;
					_append_ch(lexer, ch);
				}
				break;
			case '[':
			case ']':
				emit = _prev_state_emit[lexer->prev_state];
				emit_str = _emit(lexer);

				_append_ch(lexer, ch);
				lexer->state = PINDF_LEXER_STATE_DELIM;
				break;
			case '/':
				emit = _prev_state_emit[lexer->prev_state];
				emit_str = _emit(lexer);

				_append_ch(lexer, ch);
				lexer->state = PINDF_LEXER_STATE_IN_NAME;
				break;
			case '%':
				emit = _prev_state_emit[lexer->prev_state];
				emit_str = _emit(lexer);

				lexer->state = PINDF_LEXER_STATE_IN_COMMENT;
				break;
			default:
				if (lexer->state == PINDF_LEXER_STATE_REGULAR) {
					_append_ch(lexer, ch);
				} else if (lexer->state == PINDF_LEXER_STATE_IN_NAME) {
					_append_ch(lexer, ch);
				} else {
					emit = _prev_state_emit[lexer->prev_state];
					emit_str = _emit(lexer);

					lexer->state = PINDF_LEXER_STATE_REGULAR;
					_append_ch(lexer, ch);
				}
			}
		}
	}

	struct pindf_token *token = pindf_token_new(emit, emit_str, lexer->token_offset);
	if (emit == PINDF_LEXER_EMIT_REGULAR)
		pindf_token_regular_lex(token);
	return token;
}

struct pindf_token *pindf_token_new(int event, struct pindf_uchar_str *raw_str, uint64 offset)
{
	struct pindf_token *token = (struct pindf_token*)malloc(sizeof(struct pindf_token));
	token->event = event;
	token->raw_str = raw_str;
	token->offset = offset;
	return token;
}

void pindf_token_regular_lex(struct pindf_token *token) {
	assert(token->event == PINDF_LEXER_EMIT_REGULAR && "[regular lex] not regular token to parse");
	assert(token->raw_str && "[regular lex] raw_str is NULL");

	uchar *beg = token->raw_str->p;
	uchar *end = token->raw_str->p + token->raw_str->len;

	int reg_type = PINDF_LEXER_REGTYPE_NORM;
	int kwd_type = PINDF_KWD_UNK;

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

struct pindf_token *pindf_lex_options(struct pindf_lexer *lexer, FILE *file, uint options)
{
	struct pindf_token *token;

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