#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "uchar_str.h"

int main(int argc, const char **argv)
{
	if (argc != 2) {
		perror("Too more or too few arguments");
		fprintf(stderr, "Usage:\n\t %s [filename]", argv[0]);
		exit(0);
	}
	struct pindf_lexer *lexer = pindf_lexer_new();

	FILE *f = fopen(argv[1], "r");

	int stream_state = 0;
	int stream_len = 0;

	struct pindf_token *token = NULL;
	do {
		token = pindf_lex(lexer, f);
		switch (token->event) {
		case PINDF_LEXER_NO_EMIT:
			continue;
		case PINDF_LEXER_EMIT_WHITE_SPACE:
			printf(" ");
			break;
		case PINDF_LEXER_EMIT_EOL:
			printf("\033[2;36m(EOL)\033[0m\n");
			break;
		case PINDF_LEXER_EMIT_EOF:
			printf("(%d)", token->event);
			break;
		case PINDF_LEXER_EMIT_ERR:
			printf("(error)");
			break;
		case PINDF_LEXER_EMIT_LTR_STR:
			if (token->raw_str) {
				printf("\033[4;34m(%d:%s)\033[0m", token->event, token->raw_str->p);
			} else {
				printf("(%d)", token->event);
			}
			break;
		case PINDF_LEXER_EMIT_REGULAR:
			if (token->reg_type == PINDF_LEXER_REGTYPE_KWD) {
				printf("(%d:kwd-%d:%s)", token->event, token->kwd, token->raw_str->p);
			} else if (token->reg_type == PINDF_LEXER_REGTYPE_INT) {
				printf("(%d:int:%s)", token->event, token->raw_str->p);
			} else if (token->reg_type == PINDF_LEXER_REGTYPE_REAL) {
				printf("(%d:real:%s)", token->event, token->raw_str->p);
			} else {
				printf("(%d:norm:%s)", token->event, token->raw_str->p);
			}
			break;
		default:
			if (token->raw_str) {
				printf("(%d:%s)", token->event, token->raw_str->p);
			} else {
				printf("(%d)", token->event);
			}
		}

		if (stream_state == 0) {
			if (token->event == PINDF_LEXER_EMIT_NAME) {
				int len = strlen("/Length");
				if (token->raw_str->len == len && memcmp(token->raw_str->p, "/Length", len) == 0) {
					 stream_state = 1;
				}
			}
		} else if (stream_state == 1) {
			if (token->event == PINDF_LEXER_EMIT_REGULAR && token->reg_type == PINDF_LEXER_REGTYPE_INT) {
				stream_len = atoi((char*)token->raw_str->p);
				stream_state = 2;
			} else if (token->event == PINDF_LEXER_EMIT_WHITE_SPACE || token->event == PINDF_LEXER_EMIT_EOL) {
				;
			} else {
				perror("wrong length");
				exit(0);
			}
		} else if (stream_state == 2) {
			if (token->event == PINDF_LEXER_EMIT_REGULAR
				&& token->reg_type == PINDF_LEXER_REGTYPE_KWD
				&& token->kwd == PINDF_KWD_stream) {
				stream_state = 3;
			}
		} else if (stream_state == 3) {
			if (token->event == PINDF_LEXER_EMIT_EOL) {
				struct pindf_uchar_str *str;
				str = pindf_lex_get_stream(f, stream_len);
				pindf_uchar_str_destroy(str);
				printf("\033[4;33m<stream:%d>\033[0m", stream_len);
			} else {
				perror("stream keyword not followed by an EOL");
				exit(0);
			}
			stream_state = 0;
		}
		fflush(stdout);
	} while (token->event > 0);
	
	return 0;
}