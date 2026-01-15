#include "lexer.h"

static int _prev_state_emit[] = {
	[PINDF_LEXER_STATE_REGULAR] = PINDF_LEXER_EMIT_REGULAR,
	[PINDF_LEXER_STATE_WHITE_SPACE] = PINDF_LEXER_EMIT_WHITE_SPACE,
	[PINDF_LEXER_STATE_DELIM] = PINDF_LEXER_EMIT_DELIM,
	[PINDF_LEXER_STATE_IN_HEX_STR] = PINDF_LEXER_EMIT_HEX_STR,
	[PINDF_LEXER_STATE_IN_EOL] = PINDF_LEXER_EMIT_WHITE_SPACE,
	[PINDF_LEXER_STATE_OUT_EOL] = PINDF_LEXER_EMIT_EOL,

	[PINDF_LEXER_STATE_DEFAULT] = PINDF_LEXER_NO_EMIT,
	[PINDF_LEXER_STATE_IN_LTR_STR_ESC] = PINDF_LEXER_NO_EMIT,

	[PINDF_LEXER_STATE_IN_STREAM] = PINDF_LEXER_EMIT_ERR,
	[PINDF_LEXER_STATE_OUT_STREAM] = PINDF_LEXER_EMIT_ERR,
	[PINDF_LEXER_STATE_IN_LTR_STR] = PINDF_LEXER_EMIT_ERR,
};

void pindf_init_lexer(struct pindf_lexer *lexer)
{
	lexer->state = PINDF_LEXER_STATE_DEFAULT;
	memset(lexer->buf, 0x00, PINDF_LEXER_BUFSIZE);
	lexer->buf_end = 0;
}

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

uchar *_emit(struct pindf_lexer *lexer)
{
	uchar *emit_str = (uchar*)malloc(lexer->buf_end + 1);
	memcpy(emit_str, lexer->buf, lexer->buf_end);
	emit_str[lexer->buf_end] = '\0';
	
	memset(lexer->buf, 0x00, lexer->buf_end);
	lexer->buf_end = 0;
	
	return emit_str;
}

int prindf_lex_get_stream(struct pindf_lexer *lexer, FILE *file, int steps)
{
	if (lexer->state != PINDF_LEXER_STATE_IN_STREAM)
	{
		return -1;
	}
	uchar ch;
	for (int i = 0; i < steps; i++) {
		ch = fgetc(file);
		if (feof(file))
			return -2;
		else if (ferror(file))
			return -3;
		_append_ch(lexer, ch);
	}
	lexer->state = PINDF_LEXER_STATE_OUT_STREAM;
	return 0;
}

struct pindf_lexer_event pindf_lex(struct pindf_lexer *lexer, FILE *file)
{
	uchar ch;
	lexer->prev_state = lexer->state;
	int emit = 0;
	uchar *emit_str = NULL;
	int string_level = 0;

	while (!emit) {
		int status = (ch = fgetc(file));
		if (status < 0) {
			if (feof(file))
				emit = PINDF_LEXER_EMIT_EOF;
				break;
			if (ferror(file)) {
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
				lexer->state = PINDF_LEXER_STATE_DELIM;
				break;
			case '%':
				emit = _prev_state_emit[lexer->prev_state];
				emit_str = _emit(lexer);

				lexer->state = PINDF_LEXER_STATE_IN_COMMENT;
				break;
			default:
				if (lexer->state == PINDF_LEXER_STATE_REGULAR) {
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
	struct pindf_lexer_event ret;
	ret.event = emit;
	ret.emit_str = emit_str;
	return ret;
}