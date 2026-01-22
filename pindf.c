#include "pindf.h"

#define MATCH_INT_TOKEN_OR_ERR(result, token) \
	do { \
		if (token->event != PINDF_LEXER_EMIT_REGULAR || token->reg_type != PINDF_LEXER_REGTYPE_INT) { \
			perror("failed to parse xref"); \
			return -1; \
		} \
		result = atoll((char*)token->raw_str->p); \
	} while (0)
#define MATCH_nf_TOKEN_OR_ERR(result, token) \
	do { \
		if (token->event != PINDF_LEXER_EMIT_REGULAR || token->reg_type != PINDF_LEXER_REGTYPE_KWD) { \
			perror("failed to parse xref"); \
			return -1; \
		} \
		if (token->kwd == PINDF_KWD_n || token->kwd == PINDF_KWD_f) { \
			result = token->kwd; \
		} else { \
			perror("failed to parse xref"); \
			return -1; \
		} \
	} while (0)
#define MATCH_EOL_TOKEN_OR_ERR(token) \
	do { \
		if (token->event != PINDF_LEXER_EMIT_EOL) { \
			perror("failed to parse xref"); \
			return -1; \
		} \
	} while (0)

int _parse_xref_table(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, pindf_doc *doc)
{
	pindf_token *token = NULL;

	uint64 result;

	// pindf_xref_entry *enrtry = (pindf_xref_entry*)malloc(sizeof(pindf_xref_entry));
	pindf_xref_table *table = NULL;
	pindf_pdf_obj *trailer_obj = NULL;

	uint options = PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_NO_EMIT | PINDF_LEXER_OPT_IGNORE_WS;

	while(1) {
		size_t obj_num;
		size_t len;
		// section header
		token = pindf_lex_options(lexer, fp, options | PINDF_LEXER_OPT_IGNORE_EOL);

		if (token->event < 0) {
			return -1;
		}
		if (token->event == PINDF_LEXER_EMIT_REGULAR &&
			token->reg_type == PINDF_LEXER_REGTYPE_KWD &&
			token->kwd == PINDF_KWD_trailer
		) {
			break;
		}

		MATCH_INT_TOKEN_OR_ERR(result, token);
		obj_num = result;

		token = pindf_lex_options(lexer, fp, options);
		MATCH_INT_TOKEN_OR_ERR(result, token);
		len = result;

		token = pindf_lex_options(lexer, fp, options);
		MATCH_EOL_TOKEN_OR_ERR(token);

		table = pindf_xref_table_new(obj_num, len);

		for (int line = 0; line < len; ++line) {
			token = pindf_lex_options(lexer, fp, options);
			MATCH_INT_TOKEN_OR_ERR(result, token);
			uint64 offset = result;

			token = pindf_lex_options(lexer, fp, options);
			MATCH_INT_TOKEN_OR_ERR(result, token);
			uint gen = result;

			token = pindf_lex_options(lexer, fp, options);
			MATCH_nf_TOKEN_OR_ERR(result, token);
			int nf = result;

			pindf_xref_table_setentry(table, line, offset, gen, nf);

			token = pindf_lex_options(lexer, fp, options);
			MATCH_EOL_TOKEN_OR_ERR(token);
		}
	}

	// trailer
	int ret = pindf_parse_one_obj(parser, lexer, fp, &trailer_obj, NULL, PINDF_PDF_DICT);
	if (ret < 0)
		return -1;

	if (trailer_obj->obj_type != PINDF_PDF_DICT)
		return -1;

	doc->trailer = trailer_obj;
	doc->xref = table;

	return 0;
}

int pindf_parse_xref(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, pindf_doc *doc)
{	
	// 0 - unk
	// 1 - xref
	// 2 - stream
	int stream_xref = 0;

	int ch_int;
	uchar ch;
	pindf_token *token;
	int ret = 0;
	uint options = PINDF_LEXER_OPT_IGNORE_EOL | PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_WS | PINDF_LEXER_OPT_IGNORE_NO_EMIT;
	do {
		token = pindf_lex_options(lexer, fp, options);
		switch (token->event) {
		case PINDF_LEXER_EMIT_REGULAR:
			if (token->reg_type == PINDF_LEXER_REGTYPE_KWD && token->kwd == PINDF_KWD_xref) {
				stream_xref = 1;
			} else if (token->reg_type == PINDF_LEXER_REGTYPE_INT) {
				stream_xref = 2;
			} else {
				return -1;
			}
			break;
		default:
			return -1;
		}
	} while(stream_xref == 0);

	if (stream_xref == 1) {
		// xref table
		pindf_xref_table *table = (pindf_xref_table*)malloc(sizeof(pindf_xref_table));
		ret = _parse_xref_table(parser, lexer, fp, doc);
	} else {
		// stream xref
		pindf_parser_add_token(parser, token);
		pindf_pdf_obj *obj = NULL;
		ret = pindf_parse_one_obj(parser, lexer, fp, &obj, NULL, PINDF_PDF_IND_OBJ);
		if (ret < 0)
			return ret;

		doc->xref_stream = obj;
		ret = 0;
	}
	return ret;
}

uint64 pindf_quick_match_startxref(FILE *fp, uint64 file_len_ptr)
{
	uint64 file_len = ftell(fp);
	if (file_len_ptr == 0) {
		fseek(fp, 0, SEEK_END);
		file_len = ftell(fp);
	} else {
		file_len = file_len_ptr;
	}
	

	if (file_len < 50) {
		fseek(fp, 0, SEEK_SET);
	} else {
		fseek(fp, -50, SEEK_END);
	}

	int ch_int;
	uchar ch;
	int state = 0;
	const char *startxref = "startxref";
	uint64 startxref_start;

	while (1) {
		ch_int = fgetc(fp);
		if (ch_int < 0) {
			break;
		}
		ch = ch_int;
		// printf("state=%d, ch=%c\n", state, ch);

		switch (ch) {
		case '\0':
		case '\t':
		case 12:
		case ' ':
		case '\r':
		case '\n':
			if (state == 9) {
				state = 10;
				break;
			} else {
				state = 1;
			}
			break;
		default:
			if (state == 0)
				state = 0;
			else if (state >= 1 && ch == startxref[state-1])
				state++;
			else
			 	state = 0;
		}
		if (state == 10) {
			break;
		}
	}
	if (state != 10) {
		perror("startxref not found!");
		exit(0);
	}

	return ftell(fp);
}

int pindf_file_parse(pindf_parser *parser, FILE *fp, uint64 file_len, pindf_doc **ret_doc)
{
	assert(file_len != 0);
	size_t cur_offset_fp = ftell(fp);
	if (cur_offset_fp != 0) {
		fprintf(stderr, "[warning] file_parse not start from the beginning!");
	}

	pindf_doc *doc = pindf_doc_new("PDF-1.7");
	uint64 xref_offset;

	pindf_token *token = NULL;
	pindf_lexer *lexer = pindf_lexer_new();
	int ret = 0;

	// version
	token = pindf_lex(lexer, fp);
	if (token->event == PINDF_LEXER_EMIT_COMMENT) {
		char *p = (char*)calloc(1, token->raw_str->len + 10);
		memcpy(p, (char*)token->raw_str->p, token->raw_str->len);
		p[token->raw_str->len] = '\0';
		doc->pdf_version = p;
	} else {
		fprintf(stderr, "[warning] PDF Version not found! defaulted to %s", "PDF-1.7");
	}

	// startxref
	uint64 startxref_offset = pindf_quick_match_startxref(fp, file_len);
	uint options = PINDF_LEXER_OPT_IGNORE_EOL | PINDF_LEXER_OPT_IGNORE_NO_EMIT | PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_WS;
	while (1) {
		token = pindf_lex_options(lexer, fp, options);
		if (token->event == PINDF_LEXER_EMIT_REGULAR &&
			token->reg_type == PINDF_LEXER_REGTYPE_INT
		) {
			xref_offset = atoll((const char*)token->raw_str->p);
			break;
		} else {
			fprintf(stderr, "[error] Invalid startref!");
			return -1;
		}
	}
	doc->xref_offset = xref_offset;

	fseek(fp, xref_offset, SEEK_SET);
	// xref
	ret = pindf_parse_xref(parser, lexer, fp, doc);
	if (ret < 0) {
		fprintf(stderr, "[error] failed to parser xref table");
		return ret;
	}
	
	*ret_doc = doc;

	return 0;
}

int pindf_parse_one_obj(pindf_parser *parser, pindf_lexer *lexer, FILE *f, pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type)
{
	int stream_state = 0;
	int stream_len = 0;
	pindf_token *token = NULL;

	uint options = PINDF_LEXER_OPT_IGNORE_NO_EMIT | PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_WS | PINDF_LEXER_OPT_IGNORE_EOL;

	int ret = -1;
	do {
		
		token = pindf_lex_options(lexer, f, options);

		if (token->event < 0) {
			return -1;
		}
		if (token->event == PINDF_LEXER_EMIT_REGULAR &&
			token->reg_type == PINDF_LEXER_REGTYPE_KWD
		) {
			switch (token->kwd) {
			case PINDF_KWD_startxref:
			case PINDF_KWD_xref:
			case PINDF_KWD_trailer:
				return -3;
			default:
				;
			}
		}

		ret = pindf_parser_add_token(parser, token);

		if (stream_state == 0) {
			if (token->event == PINDF_LEXER_EMIT_NAME) {
				int len = strlen("/Length");
				if (token->raw_str->len == len && memcmp(token->raw_str->p, "/Length", len) == 0) {
					 stream_state = 1;
				}
			}

			if (parser->symbol_stack->len == 1) {
				pindf_symbol *symbol = NULL;
				pindf_vector_index(parser->symbol_stack, 0, &symbol);
				if (symbol->type == PINDF_SYMBOL_NONTERM && symbol->content.non_term->obj_type == target_type) {
					ret = 0;
					assert(symbol->content.non_term != NULL);
					*ret_obj = symbol->content.non_term;
					if (ret_offset != NULL)
						*ret_offset = symbol->offset;
					break;
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
				return -1;
			}
		} else if (stream_state == 2) {
			if (token->event == PINDF_LEXER_EMIT_REGULAR
				&& token->reg_type == PINDF_LEXER_REGTYPE_KWD
				&& token->kwd == PINDF_KWD_stream
			) {
				token = pindf_lex(lexer, f);
				if (token->event != PINDF_LEXER_EMIT_EOL) {
					perror("no EOL follow stream keyword");
					return -1;
				}
				pindf_uchar_str *stream = pindf_uchar_str_new();
				pindf_uchar_str_init(stream, stream_len);
				fread(stream->p, sizeof(uchar), stream_len, f);
				ret = pindf_parser_add_stream(parser, stream);
				if (ret < 0)
					return ret;
				lexer->offset = ftell(f);
				stream_state = 3;
			}
		} else if (stream_state == 3) {
			if (token->event == PINDF_LEXER_EMIT_REGULAR &&
				token->reg_type == PINDF_LEXER_REGTYPE_KWD &&
				token->kwd == PINDF_KWD_endstream
			) {
				stream_state = 0;
			} else {
				perror("stream keyword not followed by an EOL");
				return -1;
			}
			stream_state = 0;
		} 
	} while (token->event > 0 && ret >= 0);

	return ret;
}