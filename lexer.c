#include "lexer.h"
#include "uchar_str.h"

const char *pindf_lexer_keyword_list[] = {
	"!!!unknown!!!",
	"endobj",
	"endstream",
	"f",
	"false",
	"n",
	"null",
	"obj",
	"operators",
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

	[PINDF_LEXER_STATE_DEFAULT] = PINDF_LEXER_NO_EMIT,
	[PINDF_LEXER_STATE_IN_LTR_STR_ESC] = PINDF_LEXER_NO_EMIT,

	[PINDF_LEXER_STATE_IN_STREAM] = PINDF_LEXER_EMIT_ERR,
	[PINDF_LEXER_STATE_OUT_STREAM] = PINDF_LEXER_EMIT_ERR,
	[PINDF_LEXER_STATE_IN_LTR_STR] = PINDF_LEXER_EMIT_ERR,
};


void _append_ch(struct pindf_lexer *lexer, char ch)
{
	if (lexer->buf_end == PINDF_LEXER_BUFSIZE) {
		perror("lexer buffer overflow!");
		exit(0);
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


void pindf_init_lexer(struct pindf_lexer *lexer)
{
	lexer->state = PINDF_LEXER_STATE_DEFAULT;
	memset(lexer->buf, 0x00, PINDF_LEXER_BUFSIZE);
	lexer->buf_end = 0;
}

struct pindf_uchar_str *prindf_lex_get_stream(FILE *file, size_t len)
{
	struct pindf_uchar_str *str = pindf_uchar_str_new();
	pindf_uchar_str_init(str, len);

	fread(str->p, len, 1, file);

	return str;
}

struct pindf_token *pindf_lex(struct pindf_lexer *lexer, FILE *file)
{
	uchar ch;
	lexer->prev_state = lexer->state;
	int emit = 0;
	struct pindf_uchar_str *emit_str = NULL;
	int string_level = 0;

	while (!emit) {
		int status = (ch = fgetc(file));
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
		

		if (lexer->state == PINDF_LEXER_STATE_IN_LTR_STR) {
			switch (ch) {
			case '\\':
				lexer->state = PINDF_LEXER_STATE_IN_LTR_STR_ESC;
				break;
			case ')':
				string_level--;
				if (string_level == 0) {
					lexer->state = PINDF_LEXER_STATE_DEFAULT;
					emit = PDINF_LEXER_EMIT_LTR_STR;
					emit_str = _emit(lexer);
				} else {
					_append_ch(lexer, ch);
				}
				break;
			case '(':
				string_level++;
			default:
				_append_ch(lexer, ch);
			}
		} else if (lexer->state == PINDF_LEXER_STATE_IN_LTR_STR_ESC) {
			_append_ch(lexer, ch);
			lexer->state = PINDF_LEXER_STATE_IN_LTR_STR;
		} else if (lexer->state == PINDF_LEXER_STATE_IN_HEX_STR) {
			if (ch == '<') {
				emit = PINDF_LEXER_EMIT_DELIM;
				emit_str = _emit(lexer);

				_append_ch(lexer, '<');
				_append_ch(lexer, '<');
				
				lexer->state = PINDF_LEXER_STATE_DELIM;
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
					emit = _prev_state_emit[lexer->prev_state];
					emit_str = _emit(lexer);
					lexer->state = PINDF_LEXER_STATE_OUT_EOL;
				case '\r':
					emit = _prev_state_emit[lexer->prev_state];
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
					emit = PINDF_LEXER_EMIT_EOL;
					emit_str = _emit(lexer);
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
				string_level++;
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
					char last_char;
					if (_last_char(lexer, &ch) == 0 && last_char == '>') {
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

	struct pindf_token *token = pindf_token_new(emit, emit_str);
	if (emit == PINDF_LEXER_EMIT_REGULAR)
		pindf_token_regular_lex(token);
	return token;
}

struct pindf_token *pindf_token_new(int event, struct pindf_uchar_str *raw_str)
{
	struct pindf_token *token = (struct pindf_token*)malloc(sizeof(struct pindf_token));
	token->event = event;
	token->raw_str = raw_str;
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
	if (*beg >= '1' && *beg <= '9' || *beg == '+' || *beg == '-') {
		reg_type = PINDF_LEXER_REGTYPE_INT;
		for (uchar *p = beg + 1; p != end; p++) {
			if (!(*p >= '0' && *p <= '9')) {
				reg_type = PINDF_LEXER_REGTYPE_NORM;
				break;
			}
		}
	}
	if (reg_type != PINDF_LEXER_REGTYPE_NORM) {
		token->reg_type = reg_type;
		return;
	}
	
	// test real
	uchar *p = beg;
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
			if (*p >= '1' && *p <= '9') {
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
			}
		} else if (real_state == 3) {
			if (!(*p >= '0' && *p <= '9')) {
				reg_type = PINDF_LEXER_REGTYPE_NORM;
				break;
			}
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