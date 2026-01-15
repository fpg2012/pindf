#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

int main()
{	
	struct pindf_lexer lexer;
	pindf_init_lexer(&lexer);

	FILE *f = fopen("test.pdf", "r");

	struct pindf_lexer_event result;
	result.event = 0;
	do {
		result = pindf_lex(&lexer, f);
		if (result.event == PINDF_LEXER_NO_EMIT) {
			continue;
		} else if (result.event == PINDF_LEXER_EMIT_WHITE_SPACE) {
			printf("<%d>", result.event);
		} else if (result.event == PINDF_LEXER_EMIT_EOL) {
			printf("<%d>", result.event);
		} else if (result.event == PINDF_LEXER_EMIT_EOF) {
			printf("<%d>", result.event);
		} else {
			printf("<%d:%s>", result.event, result.emit_str);
		}
	} while (result.event != PINDF_LEXER_EMIT_EOF);
	
	return 0;
}