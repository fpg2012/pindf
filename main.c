#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

int main()
{	
	struct pindf_lexer lexer;
	pindf_init_lexer(&lexer);

	FILE *f = fopen("test.pdf", "r");

	struct pindf_token *token = NULL;
	do {
		token = pindf_lex(&lexer, f);
		switch (token->event) {
		case PINDF_LEXER_NO_EMIT:
			continue;
		case PINDF_LEXER_EMIT_WHITE_SPACE:
		case PINDF_LEXER_EMIT_EOL:
		case PINDF_LEXER_EMIT_EOF:
			printf("<%d>", token->event);
			break;
		case PINDF_LEXER_EMIT_ERR:
			printf("<error>");
			break;
		case PINDF_LEXER_EMIT_REGULAR:
			if (token->reg_type == PINDF_LEXER_REGTYPE_KWD) {
				printf("<%d:kwd%d:%s>", token->event, token->kwd, token->raw_str->p);
			} else if (token->reg_type == PINDF_LEXER_REGTYPE_INT) {
				printf("<%d:int:%s>", token->event, token->raw_str->p);
			} else if (token->reg_type == PINDF_LEXER_REGTYPE_REAL) {
				printf("<%d:real:%s>", token->event, token->raw_str->p);
			} else {
				printf("<%d:norm:%s>", token->event, token->raw_str->p);
			}
			break;
		default:
			if (token->raw_str) {
				printf("<%d:%s>", token->event, token->raw_str->p);
			} else {
				printf("<%d>", token->event);
			}
		}
	} while (token->event > 0);
	
	return 0;
}