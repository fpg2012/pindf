#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "pdf_obj.h"
#include "simple_vector.h"
#include "uchar_str.h"


int main(int argc, const char **argv)
{
	if (argc != 2) {
		perror("Too more or too few arguments");
		fprintf(stderr, "Usage:\n\t %s [filename]", argv[0]);
		exit(0);
	}
	struct pindf_lexer *lexer = pindf_lexer_new();
	struct pindf_parser *parser = pindf_parser_new();

	FILE *f = fopen(argv[1], "r");

	int stream_state = 0;
	int stream_len = 0;

	int ret = -1;

	struct pindf_token *token = NULL;

	uint64 file_len = 0;

	fseek(f, 0, SEEK_END);
	file_len = ftell(f);

	assert(file_len > 0);

	// === quick parse test ===
	printf("=== quick parse test ===\n");
	fseek(f, 0, SEEK_SET);
	pindf_parser_file_parse(parser, f, file_len);
	printf("file_len: %llu\n", file_len);
	
	fseek(f, 0, SEEK_SET);

	// === full parse test ===
	printf("=== full parse test ===\n");
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

		if (token->event == -1) {
			break;
		}

		ret = pindf_parser_add_token(parser, token);

		if (ret == -1) {
			printf("exit\n");
		} else if (ret == -2) {
			printf("ret-2\n");
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
				struct pindf_uchar_str *str = pindf_lex_get_stream(f, stream_len);
				
				ret = pindf_parser_add_stream(parser, str);
			} else {
				perror("stream keyword not followed by an EOL");
				exit(0);
			}
			stream_state = 0;
		}
		fflush(stdout);
	} while (token->event > 0 && ret >= 0);

	// === serialize to json ===
	printf("\n=== serialize to json ===\n");
	uint64 obj_stack_len = parser->symbol_stack->len;
	
	struct pindf_symbol *temp_symbol;
	char *buf = (char*)malloc(file_len * 2);
	char *p = buf;
	p += sprintf(p, "[\n");
	for (uint64 i = 0; i < obj_stack_len; ++i) {
		pindf_vector_index(parser->symbol_stack, i, &temp_symbol);
		char *r = p;
		if (temp_symbol->type == PINDF_SYMBOL_TERM) {
			p += sprintf(p, "{\"type\":\"term\",\"raw\":\"");
			uchar *q = temp_symbol->content.term->raw_str->p, *end = q + temp_symbol->content.term->raw_str->len;
			while (q != end) {
				if (*q == '\n') {
					p += sprintf(p, "\\n");
				} else if (*q == '\r') {
					p += sprintf(p, "\\r");	
				} else if (*q == '\0') {
					p += sprintf(p, "\\0");
				} else if (*q == '\"') {
					p += sprintf(p, "\\\"");
				} else if (*q == '\\') {
					p += sprintf(p, "\\\\");
				} else if (*q > 0x80) {
					p += sprintf(p, "\\x%02x", (uint)*q);
				} else {
					*(p++) = *q;
				}
				q++;
			}
			p += sprintf(p, "\"}");
			
		} else {
			p = pindf_pdf_obj_serialize_json(temp_symbol->content.non_term, p);
		}

		if (i < obj_stack_len - 1) {
			p += sprintf(p, ",\n");
		}

		printf("\033[4;34m%s\033[0m\nrange: %ld-%ld\n", r, r - buf, p - buf);

		printf("=== cur buf ===\n%s================\n\n", buf);
	}
	p += sprintf(p, "\n]\n");

	FILE *out = fopen("out.json", "wb");
	// fwrite(buf, sizeof(char), p - buf, stdout);
	fwrite(buf, sizeof(char), p - buf, out);
	
	return 0;
}