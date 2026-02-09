#include "pindf.h"
#include "container/uchar_str.h"
#include "core/lexer.h"
#include "core/parser.h"
#include "logger/logger.h"
#include "pdf/doc.h"
#include "pdf/obj.h"

#define MATCH_INT_TOKEN_OR_ERR(result, token) \
	do { \
		if (token->event != PINDF_LEXER_EMIT_REGULAR || token->reg_type != PINDF_LEXER_REGTYPE_INT) { \
			PINDF_ERR("failed to parse xref, failed to match integer"); \
			pindf_token_destroy(token); \
			free(token); \
			return -1; \
		} \
		result = atoll((char*)token->raw_str->p); \
		pindf_token_destroy(token); \
		free(token); \
	} while (0)
#define MATCH_nf_TOKEN_OR_ERR(result, token) \
	do { \
		if (token->event != PINDF_LEXER_EMIT_REGULAR || token->reg_type != PINDF_LEXER_REGTYPE_KWD) { \
			PINDF_ERR("failed to parse xref, failed to match keyword"); \
			pindf_token_destroy(token); \
			free(token); \
			return -1; \
		} \
		if (token->kwd == PINDF_KWD_n || token->kwd == PINDF_KWD_f) { \
			result = token->kwd; \
		} else { \
			PINDF_ERR("failed to parse xref, invalid keyword"); \
			pindf_token_destroy(token); \
			free(token); \
			return -1; \
		} \
		pindf_token_destroy(token); \
		free(token); \
	} while (0)
#define MATCH_EOL_TOKEN_OR_ERR(token) \
	do { \
		if (token->event != PINDF_LEXER_EMIT_EOL) { \
			PINDF_ERR("failed to parse xref"); \
			pindf_token_destroy(token); \
			free(token); \
			return -1; \
		} \
		pindf_token_destroy(token); \
		free(token); \
	} while (0)

#define GET_VALUE_OR_ERR(key, var, val, TYPE) \
	do { \
		temp_obj = pindf_dict_getvalue2(&trailer, key); \
		if (temp_obj == NULL || temp_obj->obj_type != PINDF_PDF_##TYPE) { \
			PINDF_ERR("invalid type of %s: %d", key, PINDF_PDF_##TYPE); \
			return -1; \
		} \
		var = temp_obj->content.val; \
	} while (0)

#define GET_VALUE_OR_DEFAULT(key, var, val, TYPE, def) \
do { \
	temp_obj = pindf_dict_getvalue2(&trailer, key); \
	if (temp_obj != NULL && temp_obj->obj_type != PINDF_PDF_##TYPE) { \
		PINDF_ERR("invalid type of %s: %d", key, PINDF_PDF_##TYPE); \
		return -1; \
	} \
	if (temp_obj == NULL) { \
		var = def; \
	} else { \
		var = temp_obj->content.val; \
	} \
} while (0)

int _parse_xref_table(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, pindf_doc *doc, int *ret_prev)
{
	size_t init_fp = ftell(fp);

	pindf_token *token = NULL;
	enum pindf_lexer_event token_event = 0;
	enum pindf_kwd token_kwd;

	uint64 result;

	// parse trailer first
	pindf_pdf_obj *trailer_obj = NULL;

	uint options = PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_NO_EMIT | PINDF_LEXER_OPT_IGNORE_WS | PINDF_LEXER_EMIT_EOL;

	// quick match trailer keyword
	do {
		// section header
		token = pindf_lex_options(lexer, fp, options | PINDF_LEXER_OPT_IGNORE_EOL);
		token_event = token->event;
		token_kwd = token->kwd;
		pindf_token_destroy(token);
		free(token);

		if (token_event < 0) {
			PINDF_ERR("failed to parse xref table, unexpected lexer error");
			return -1;
		}
	} while (token_kwd != PINDF_KWD_trailer);

	// trailer
	int ret = pindf_parse_one_obj(parser, lexer, fp, &trailer_obj, NULL, PINDF_PDF_DICT);
	if (ret < 0) {
		PINDF_ERR("failed to parse trailer dict, not a valid PDF object");
		return -1;
	}

	if (trailer_obj->obj_type != PINDF_PDF_DICT) {
		PINDF_ERR("trailer is not a dict!");
		return -1;
	}

	pindf_pdf_dict trailer = trailer_obj->content.dict;

	// first time set trailer
	if (doc->xref == NULL) {
		doc->trailer = trailer_obj->content.dict;

		// get size
		pindf_pdf_obj *size_obj = NULL;
		size_obj = pindf_dict_getvalue2(&trailer, "/Size");
		if (size_obj == NULL || size_obj->obj_type != PINDF_PDF_INT) {
			PINDF_ERR("invalid type of /Size in trailer");
			return -1;
		}
		size_t size = size_obj->content.num;
		
		// init xref
		doc->xref = (pindf_xref*)malloc(sizeof(pindf_xref));
		pindf_xref_init(doc->xref, size);
	}

	// get prev (default to -1)
	{
		pindf_pdf_obj *temp_obj = NULL;
		int prev_offset = -1;
		temp_obj = pindf_dict_getvalue2(&trailer, "/Prev");
		if (temp_obj != NULL) {
			if (temp_obj->obj_type != PINDF_PDF_INT) {
				PINDF_ERR("invalid type of /Prev in trailer");
				return -1;
			}
			prev_offset = temp_obj->content.num;
		}
		*ret_prev = prev_offset;
	}

	fseek(fp, init_fp, SEEK_SET);
	options = PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_NO_EMIT | PINDF_LEXER_OPT_IGNORE_WS;

	while(1) {
		size_t obj_num;
		size_t len;
		// section header
		token = pindf_lex_options(lexer, fp, options | PINDF_LEXER_OPT_IGNORE_EOL);
		token_event = token->event;
		token_kwd = token->kwd;

		if (token_event < 0) {
			PINDF_ERR("failed to parse xref table, unexpected lexer error");
			pindf_token_destroy(token);
			free(token);
			return -1;
		}
		if (token->kwd == PINDF_KWD_trailer) {
			pindf_token_destroy(token);
			free(token);
			break;
		}

		MATCH_INT_TOKEN_OR_ERR(result, token);
		obj_num = result;

		token = pindf_lex_options(lexer, fp, options);
		MATCH_INT_TOKEN_OR_ERR(result, token);
		len = result;

		token = pindf_lex_options(lexer, fp, options);
		MATCH_EOL_TOKEN_OR_ERR(token);

		pindf_xref_section *section = pindf_xref_alloc_section(doc->xref, obj_num, len);

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

			pindf_xref_section_setentry(section, line, offset, gen, (nf == PINDF_KWD_n ? PINDF_XREF_ENTRY_N : PINDF_XREF_ENTRY_F));

			token = pindf_lex_options(lexer, fp, options);
			MATCH_EOL_TOKEN_OR_ERR(token);
		}
	}

	return 0;
}

// NOTE on ret value
// 0 - normal
// 1 - the stream is not encoded
int pindf_stream_decode(pindf_pdf_obj *stream, pindf_uchar_str *decoded)
{
	assert(stream->obj_type == PINDF_PDF_STREAM);
	pindf_pdf_dict stream_dict = stream->content.stream.dict->content.dict;

	// filter
	pindf_pdf_obj *filters = pindf_dict_getvalue2(&stream_dict, "/Filter");

	// decode params
	pindf_pdf_obj *decode_params_obj = NULL;
	decode_params_obj = pindf_dict_getvalue2(&stream_dict, "/DecodeParms");
	if (decode_params_obj != NULL && 
		decode_params_obj->obj_type != PINDF_PDF_DICT && 
		decode_params_obj->obj_type != PINDF_PDF_ARRAY
	) {
		return -1;
	}

	// parse filter and decode params
	enum pindf_filter_type filter_array[10];
	pindf_pdf_dict *decode_param_array[10];
	memset(filter_array, 0, sizeof(enum pindf_filter_type) * 10);
	memset(decode_param_array, 0, sizeof(pindf_pdf_dict*) * 10);
	int n_filters = 0;

	if (filters != NULL) {
		if (filters->obj_type == PINDF_PDF_NAME) {
			filter_array[0] = pindf_filter_type_from_name(filters->content.name);
			n_filters = 1;

			if (decode_params_obj != NULL) {
				if (decode_params_obj->obj_type != PINDF_PDF_DICT) {
					PINDF_ERR("decode params type invalid");
					return -1;
				}
				decode_param_array[0] = &decode_params_obj->content.dict;
			}
		} else if (filters->obj_type == PINDF_PDF_ARRAY) {
			n_filters = filters->content.array->len;
			for (int i = 0; i < n_filters; ++i) {
				pindf_pdf_obj *temp2;
				pindf_vector_index(filters->content.array, i, &temp2);
				if (temp2->obj_type != PINDF_PDF_NAME) {
					return -1;
				}
				filter_array[i] = pindf_filter_type_from_name(temp2->content.name);
			}

			if (decode_params_obj != NULL) {
				if (decode_params_obj->obj_type != PINDF_PDF_ARRAY ||
					decode_params_obj->content.array->len != n_filters
				) {
					PINDF_ERR("decode params type invalid");
					return -1;
				}
				for (int i = 0; i < n_filters; ++i) {
					pindf_pdf_obj *temp3;
					pindf_vector_index(decode_params_obj->content.array, i, &temp3);
					if (temp3->obj_type != PINDF_PDF_DICT) {
						PINDF_ERR("decode params type invalid");
						return -1;
					}
					decode_param_array[i] = &temp3->content.dict;
				}
			}
		} else {
			return -1;
		}
	}

	if (n_filters == 0) {
		*decoded = *stream->content.stream.stream_content;
		return 1;
	}

	// init buffers
	pindf_uchar_str buffer1, buffer2;
	pindf_uchar_str_init(&buffer1, PINDF_STREAM_BUF_LEN);
	pindf_uchar_str_init(&buffer2, PINDF_STREAM_BUF_LEN);

	pindf_uchar_str *buffer_in = stream->content.stream.stream_content;
	pindf_uchar_str *buffer_out = &buffer1;

	// decode
	for (int i = 0; i < n_filters; ++i) {
		if (i >= 1) {
			buffer_in = buffer_out;
			buffer_out = (buffer_out == &buffer1 ? &buffer2 : &buffer1);
		}

		pindf_stream_filter _f;
		int ret = 0;

		ret = pindf_filter_init(&_f, filter_array[i], decode_param_array[i]);
		if (ret < 0) {
			PINDF_ERR("failed to init filter %d", filter_array[i]);
			return ret;
		}

		ret = _f.decode(&_f, buffer_out, buffer_in);
		if (ret < 0) {
			PINDF_ERR("failed to decode xref stream with filter %d", filter_array[i]);
			return -1;
		}
	}

	*decoded = *buffer_out;

	if (buffer_out != &buffer1) {
		pindf_uchar_str_destroy(&buffer1);
	}
	if (buffer_out != &buffer2) {
		pindf_uchar_str_destroy(&buffer2);
	}
	return 0;
}

int pindf_parse_xref_obj(pindf_doc *doc, pindf_pdf_obj *obj, int *ret_prev)
{
	pindf_pdf_obj *xref_stream = obj->content.indirect_obj.obj;
	if (xref_stream->obj_type != PINDF_PDF_STREAM) {
		PINDF_ERR("xref stream is not a stream!");
		return -1;
	}
	pindf_pdf_dict trailer = xref_stream->content.stream.dict->content.dict;

	if (doc->xref == NULL) {
		doc->trailer = trailer;
	}

	uint size;
	uint w[3];
	int prev_offset;
	pindf_vector *index_pairs_vec = NULL;
	int index_pairs[512];
	int index_n_pairs = 0;

	pindf_pdf_obj *temp_obj = NULL;
	
	// size
	GET_VALUE_OR_ERR("/Size", size, num, INT);
	
	// prev_offset
	GET_VALUE_OR_DEFAULT("/Prev", prev_offset, num, INT, -1);
	PINDF_DEBUG("prev: %d", prev_offset);

	// W
	temp_obj = pindf_dict_getvalue2(&trailer, "/W");
	if (temp_obj == NULL || temp_obj->obj_type != PINDF_PDF_ARRAY || temp_obj->content.array->len != 3) {
		return -1;
	}
	for (int i = 0; i < 3; ++i) {
		pindf_pdf_obj *temp2;
		pindf_vector_index(temp_obj->content.array, i, &temp2);
		if (temp2->obj_type != PINDF_PDF_INT) {
			return -1;
		}
		w[i] = temp2->content.num;
	}

	// index pairs
	GET_VALUE_OR_DEFAULT("/Index", index_pairs_vec, array, ARRAY, NULL);
	if (index_pairs_vec == NULL) {
		index_pairs[0] = 0;
		index_pairs[1] = size;
		index_n_pairs = 1;
	} else {
		pindf_pdf_obj *temp2 = NULL;
		index_n_pairs = index_pairs_vec->len / 2;
		for (int i = 0; i < index_pairs_vec->len; ++i) {
			pindf_vector_index(index_pairs_vec, i, &temp2);
			if (temp2->obj_type != PINDF_PDF_INT) {
				return -1;
			}
			index_pairs[i] = temp2->content.num;
		}
	}
	
	pindf_uchar_str buffer_out;
	int result_of_decode = pindf_stream_decode(xref_stream, &buffer_out);
	if (result_of_decode < 0) {
		PINDF_ERR("failed to decode stream");
		return result_of_decode;
	}
	
	// fill in the xref section
	int n_entries = buffer_out.len / (w[0] + w[1] + w[2]);
	uchar *q = buffer_out.p;
	uint64 temp_arr[3] = {0, 0, 0};

	if (doc->xref == NULL) {
		doc->xref = (pindf_xref*)malloc(sizeof(pindf_xref));
		pindf_xref_init(doc->xref, size);
	}
	pindf_xref_section *section = NULL;
	int cur_section = 0, cur_section_first_entry = 0;
	
	for (int i = 0; i < n_entries; ++i) {
		if (i == cur_section_first_entry) {
			PINDF_DEBUG("alloc section %d (st=%d len=%d)", cur_section, index_pairs[cur_section * 2], index_pairs[cur_section * 2 + 1]);
			section = pindf_xref_alloc_section(doc->xref, index_pairs[cur_section * 2], index_pairs[cur_section * 2 + 1]);
		}

		memset(temp_arr, 0x00, sizeof(temp_arr));
		for (int j = 0; j < 3; ++j) {
			for (int k = 0; k < w[j]; ++k, ++q) {
				temp_arr[j] = (temp_arr[j] << 8) + *q;
			}
		}
		PINDF_DEBUG("%llx %llx %llx", temp_arr[0], temp_arr[1], temp_arr[2]);
		pindf_xref_section_setentry(section, i - cur_section_first_entry, temp_arr[1], temp_arr[2], temp_arr[0]);

		if (i + 1 >= cur_section_first_entry + index_pairs[cur_section * 2 + 1]) {
			PINDF_DEBUG("add section %d", cur_section);
			++cur_section;
			cur_section_first_entry = i + 1;
		}
	}

	*ret_prev = prev_offset;

	if (result_of_decode == 0) {
		// buf is used for decoding
		pindf_uchar_str_destroy(&buffer_out);
	}

	return 0;
}

int pindf_parse_xref(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, pindf_doc *doc, uint64 startxref)
{	
	int prev = (int)startxref;
	int ret = 0;
	do {
		PINDF_DEBUG("parse xref at offset %d", prev);
		pindf_parser_clear(parser);
		pindf_lexer_clear(lexer);
		fseek(fp, prev, SEEK_SET);
		// 0 - unk
		// 1 - xref
		// 2 - stream
		int stream_xref = 0;

		int ch_int;
		uchar ch;
		pindf_token *token;
		
		uint options = PINDF_LEXER_OPT_IGNORE_EOL | PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_WS | PINDF_LEXER_OPT_IGNORE_NO_EMIT;
		do {
			token = pindf_lex_options(lexer, fp, options);
			switch (token->event) {
			case PINDF_LEXER_EMIT_REGULAR:
				if (token->reg_type == PINDF_LEXER_REGTYPE_KWD && token->kwd == PINDF_KWD_xref) {
					stream_xref = 1;
					pindf_token_destroy(token);
					free(token);
				} else if (token->reg_type == PINDF_LEXER_REGTYPE_INT) {
					stream_xref = 2;
					// no need to free token, it will be used later
				} else {
					PINDF_ERR("invalid xref, neither table nor stream");
					pindf_token_destroy(token);
					free(token);
					return -1;
				}
				break;
			default:
				PINDF_ERR("invalid xref, neither table nor stream");
				pindf_token_destroy(token);
				free(token);
				return -1;
			}
		} while(stream_xref == 0);

		if (stream_xref == 1) {
			// xref table
			ret = _parse_xref_table(parser, lexer, fp, doc, &prev);
			if (ret < 0) {
				PINDF_ERR("failed to parse xref table");
				return ret;
			}
		} else {
			// stream xref
			pindf_parser_add_token(parser, token);
			pindf_token_destroy(token);
			free(token);
			
			pindf_pdf_obj *obj = NULL;
			ret = pindf_parse_one_obj(parser, lexer, fp, &obj, NULL, PINDF_PDF_IND_OBJ);
			if (ret < 0) {
				return ret;
			}
			ret = pindf_parse_xref_obj(doc, obj, &prev);
			if (ret < 0) {
				PINDF_ERR("failed to parse xref stream object");
			}
		}
	} while (prev != -1 && ret >= 0);
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
		PINDF_ERR("startxref not found!");
		exit(0);
	}

	return ftell(fp);
}

int pindf_file_parse(pindf_parser *parser, pindf_lexer *lexer, FILE *fp, uint64 file_len, pindf_doc **ret_doc)
{
	assert(file_len != 0);
	size_t cur_offset_fp = ftell(fp);
	if (cur_offset_fp != 0) {
		PINDF_WARN("file_parse not start from the beginning!");
	}

	pindf_parser_clear(parser);
	pindf_lexer_clear(lexer);

	pindf_doc *doc = pindf_doc_new("PDF-1.7", fp);
	uint64 xref_offset;

	pindf_token *token = NULL;
	int ret = 0;

	// version
	token = pindf_lex(lexer, fp);
	if (token->event == PINDF_LEXER_EMIT_COMMENT) {
		char *p = (char*)calloc(1, token->raw_str->len + 10);
		memcpy(p, (char*)token->raw_str->p, token->raw_str->len);
		p[token->raw_str->len] = '\0';
		doc->pdf_version = p;
	} else {
		pindf_token_destroy(token);
		PINDF_WARN("PDF Version not found! defaulted to %s", "PDF-1.7");
	}
	free(token);

	// startxref
	uint64 startxref_offset = pindf_quick_match_startxref(fp, file_len);
	uint options = PINDF_LEXER_OPT_IGNORE_EOL | PINDF_LEXER_OPT_IGNORE_NO_EMIT | PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_WS;
	while (1) {
		token = pindf_lex_options(lexer, fp, options);
		if (token->reg_type == PINDF_LEXER_REGTYPE_INT) {
			xref_offset = atoll((const char*)token->raw_str->p);
			pindf_token_destroy(token);
			free(token);
			break;
		} else {
			PINDF_ERR("Invalid startref!");
			pindf_token_destroy(token);
			free(token);
			return -1;
		}
	}
	doc->xref_offset = xref_offset;

	// xref
	ret = pindf_parse_xref(parser, lexer, fp, doc, xref_offset);
	if (ret < 0) {
		PINDF_ERR("failed to parse xref");
		return ret;
	}
	
	*ret_doc = doc;

	return 0;
}

int _stream_state_handling(
	pindf_parser *parser, pindf_lexer *lexer, FILE *f,
	pindf_token *token, enum pindf_lexer_event token_event,
	int *stream_state, int *stream_len,
	pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type)
{
	int ret = 0;
	if (*stream_state == 0) {
		if (token_event == PINDF_LEXER_EMIT_NAME) {
			int len = strlen("/Length");
			if (token->raw_str->len == len && memcmp(token->raw_str->p, "/Length", len) == 0) {
				*stream_state = 1;
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
				return 1;
			}
		}
	} else if (*stream_state == 1) {
		if (token_event == PINDF_LEXER_EMIT_REGULAR && token->reg_type == PINDF_LEXER_REGTYPE_INT) {
			*stream_len = atoi((char*)token->raw_str->p);
			*stream_state = 2;
		} else if (token_event == PINDF_LEXER_EMIT_WHITE_SPACE || token_event == PINDF_LEXER_EMIT_EOL) {
			;
		} else {
			PINDF_ERR("stream wrong length");
			return -1;
		}
	} else if (*stream_state == 2) {
		if (token_event == PINDF_LEXER_EMIT_REGULAR
			&& token->reg_type == PINDF_LEXER_REGTYPE_KWD
			&& token->kwd == PINDF_KWD_stream
		) {
			pindf_token *eol_token = pindf_lex(lexer, f);
			token_event = eol_token->event;
			if (token_event != PINDF_LEXER_EMIT_EOL) {
				PINDF_ERR("no EOL follow stream keyword");
				pindf_token_destroy(eol_token);
				free(eol_token);
				return -1;
			}
			pindf_token_destroy(eol_token);
			free(eol_token);

			pindf_uchar_str *stream = pindf_uchar_str_new();
			pindf_uchar_str_init(stream, *stream_len);
			size_t content_offset = ftell(f);
			fread(stream->p, sizeof(uchar), *stream_len, f);
			ret = pindf_parser_add_stream(parser, stream, content_offset);
			if (ret < 0)
				return ret;
			lexer->offset = ftell(f);
			*stream_state = 3;
		}
	} else if (*stream_state == 3) {
		if (token_event == PINDF_LEXER_EMIT_REGULAR &&
			token->reg_type == PINDF_LEXER_REGTYPE_KWD &&
			token->kwd == PINDF_KWD_endstream
		) {
			*stream_state = 0;
		} else {
			PINDF_ERR("stream keyword not followed by an EOL");
			return -1;
		}
	}
	return 0;
}

void _free_token_after_parse(pindf_token *token, enum pindf_lexer_event token_event)
{
	switch (token_event) {
	case PINDF_LEXER_EMIT_HEX_STR:
	case PINDF_LEXER_EMIT_LTR_STR:
	case PINDF_LEXER_EMIT_NAME:
		free(token);
		break;
	case PINDF_LEXER_EMIT_DELIM:
		break;
	default:
		pindf_token_destroy(token);
		free(token);
	}
}

int pindf_parse_one_obj(pindf_parser *parser, pindf_lexer *lexer, FILE *f, pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type)
{
	int stream_state = 0;
	int stream_len = 0;
	pindf_token *token = NULL;
	int token_event = 0;

	uint options = PINDF_LEXER_OPT_IGNORE_NO_EMIT | PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_WS | PINDF_LEXER_OPT_IGNORE_EOL;

	int ret = -1;
	do {
		
		token = pindf_lex_options(lexer, f, options);
		token_event = token->event;

		if (token_event < 0) {
			PINDF_ERR("failed to parse object, unexpected lexer error");
			pindf_token_destroy(token);
			free(token);
			return -1;
		}
		if (token_event == PINDF_LEXER_EMIT_REGULAR &&
			token->reg_type == PINDF_LEXER_REGTYPE_KWD
		) {
			switch (token->kwd) {
			case PINDF_KWD_startxref:
			case PINDF_KWD_xref:
			case PINDF_KWD_trailer:
				PINDF_ERR("unexpected keyword while parsing object: %d", token->kwd);
				pindf_token_destroy(token);
				free(token);
				return -3;
			default:
				;
			}
		}

		ret = pindf_parser_add_token(parser, token);
		if (ret < 0) {
			PINDF_ERR("failed to parse object, parser error");
			pindf_token_destroy(token);
			free(token);
			return ret;
		}

		ret = _stream_state_handling(parser, lexer, f, token, token_event, &stream_state, &stream_len, ret_obj, ret_offset, target_type);
		if (ret < 0)
			return ret;

		_free_token_after_parse(token, token_event);
	} while (token_event > 0 && ret == 0);

	return ret;
}

int pindf_parse_one_obj_from_buffer(
	pindf_parser *parser, pindf_lexer *lexer,
	pindf_uchar_str *buffer, size_t offset,
	pindf_pdf_obj **ret_obj, uint64 *ret_offset, int target_type)
{
	int stream_state = 0;
	int stream_len = 0;
	pindf_token *token = NULL;
	int token_event = 0;

	uint options = PINDF_LEXER_OPT_IGNORE_NO_EMIT | PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_WS | PINDF_LEXER_OPT_IGNORE_EOL;
	int chars_read = 0;
	size_t cursor = 0;

	int ret = -1;
	do {
		token = pindf_lex_from_buffer_options(lexer, buffer->p, offset + cursor, buffer->len, &chars_read, options);
		cursor += chars_read;
		token_event = token->event;

		if (token_event < 0) {
			PINDF_ERR("failed to parse object, unexpected lexer error %d", token_event);
			pindf_token_destroy(token);
			free(token);
			return -1;
		}
		if (token_event == PINDF_LEXER_EMIT_REGULAR &&
			token->reg_type == PINDF_LEXER_REGTYPE_KWD
		) {
			switch (token->kwd) {
			case PINDF_KWD_startxref:
			case PINDF_KWD_xref:
			case PINDF_KWD_trailer:
				PINDF_ERR("unexpected keyword while parsing object: %d", token->kwd);
				pindf_token_destroy(token);
				free(token);
				return -3;
			default:
				;
			}
		}

		ret = pindf_parser_add_token(parser, token);
		if (ret < 0) {
			pindf_token_destroy(token);
			free(token);
			PINDF_ERR("failed to parse object, parser error");
			return ret;
		}

		if (parser->symbol_stack->len == 1) {
			pindf_symbol *symbol = NULL;
			pindf_vector_index(parser->symbol_stack, 0, &symbol);
			if (symbol->type == PINDF_SYMBOL_NONTERM && 
				(target_type == PINDF_PDF_UNK || symbol->content.non_term->obj_type == target_type)) {
				assert(symbol->content.non_term != NULL);
				*ret_obj = symbol->content.non_term;
				if (ret_offset != NULL) 
					*ret_offset = symbol->offset;

				_free_token_after_parse(token, token_event);
				break;
			}
		}

		// NOTE: this function is for compressed object parser, not for support normal file parsing from buffer
		// stream object are not allowed in another stream, so we do not need to handle stream state here

		_free_token_after_parse(token, token_event);
	} while (token_event > 0 && ret >= 0);

	return ret;
}

pindf_pdf_obj *pindf_doc_getobj(pindf_doc *doc, pindf_parser *parser, pindf_lexer *lexer, uint64 obj_num)
{
	assert(doc != NULL);
	assert(doc->xref != NULL);

	pindf_parser_clear(parser);
	pindf_lexer_clear(lexer);

	if (obj_num >= doc->xref->size) {
		// invalid obj num
		PINDF_WARN("obj_num %d out of xref table size %zu", obj_num, doc->xref->size);
		return NULL;
	}

	// init obj entry
	if (doc->ind_obj_list == NULL) {
		doc->ind_obj_list = (pindf_obj_entry*)calloc(doc->xref->size, sizeof(pindf_obj_entry));
	}

	if (doc->ind_obj_list[obj_num].number == 0) {
		// not loaded yet
		pindf_parser_clear(parser);
		pindf_lexer_clear(lexer);
		// look up xref table
		pindf_xref_entry *entry = &doc->xref->entries[obj_num];
		if (entry->type == PINDF_XREF_ENTRY_N) {
			uint64 offset = entry->fields[0];
			fseek(doc->fp, offset, SEEK_SET);
			pindf_pdf_obj *obj = NULL;
			uint64 obj_offset = 0;
			int ret = pindf_parse_one_obj(parser, lexer, doc->fp, &obj, &obj_offset, PINDF_PDF_IND_OBJ);
			if (ret < 0) {
				PINDF_ERR("failed to parse indirect obj %d at offset %llu", obj_num, offset);
				return NULL;
			}
			if (obj == NULL) {
				PINDF_ERR("failed to parse obj %d, the result is NULL");
			}
			doc->ind_obj_list[obj_num] = (pindf_obj_entry){
				.available = PINDF_OBJ_AVAILABLE,
				.offset = obj_offset,
				.number = obj_num,
				.ind_obj = obj,
			};
		} else if (entry->type == PINDF_XREF_ENTRY_C) {
			pindf_pdf_obj *obj_stream = pindf_doc_getobj(doc, parser, lexer, entry->fields[0]);
			if (obj_stream == NULL || obj_stream->obj_type != PINDF_PDF_IND_OBJ) {
				PINDF_ERR("failed to get object stream for compressed object %d, object stream not found", obj_num);
				return NULL;
			}
			obj_stream = obj_stream->content.indirect_obj.obj;
			if (obj_stream == NULL || obj_stream->obj_type != PINDF_PDF_STREAM) {
				PINDF_ERR("failed to get object stream for compressed object %d, object stream not found, expect obj stream, but got %d", obj_num, obj_stream->obj_type);
				return NULL;
			}

			// pindf_uchar_str *stream_buffer = obj_stream->content.stream.stream_content;
			int n = 0, first = 0;

			// check type
			pindf_pdf_obj *temp_obj = pindf_dict_getvalue2(&obj_stream->content.stream.dict->content.dict, "/Type");
			if (temp_obj == NULL || temp_obj->obj_type != PINDF_PDF_NAME) {
				PINDF_ERR("object stream has no /Type");
				return NULL;
			}
			if (pindf_uchar_str_cmp3(temp_obj->content.name, "/ObjStm") != 0) {
				PINDF_ERR("object stream /Type is not /ObjStm");
				return NULL;
			}
			// get n
			temp_obj = pindf_dict_getvalue2(&obj_stream->content.stream.dict->content.dict, "/N");
			if (temp_obj == NULL || temp_obj->obj_type != PINDF_PDF_INT) {
				PINDF_ERR("object stream has no /N");
				return NULL;
			}
			n = temp_obj->content.num;
			// get first
			temp_obj = pindf_dict_getvalue2(&obj_stream->content.stream.dict->content.dict, "/First");
			if (temp_obj == NULL || temp_obj->obj_type != PINDF_PDF_INT) {
				PINDF_ERR("object stream has no /First");
				return NULL;
			}
			first = temp_obj->content.num;
			
			PINDF_DEBUG("object stream has %d objects, first at %d", n, first);

			// decode stream
			pindf_uchar_str decoded;
			int result_of_decode = pindf_stream_decode(obj_stream, &decoded);
			if (result_of_decode < 0) {
				PINDF_ERR("failed to decode obj stream");
				return NULL;
			}
			PINDF_DEBUG("decoded:\n %s", decoded.p);

			// parse object stream header
			pindf_token *token = NULL;
			int chars_read = 0;
			int state = 0; // 0 - obj num, 1 - offset
			int nums[2] = {0, 0};
			uint options = PINDF_LEXER_OPT_IGNORE_CMT | PINDF_LEXER_OPT_IGNORE_EOL | PINDF_LEXER_OPT_IGNORE_WS | PINDF_LEXER_OPT_IGNORE_NO_EMIT;
			size_t cursor = 0;

			bool found_flag = false;
			for (int i = 0; i < n; ++i) {
				for (int j = 0; j < 2; ++j) {
					token = pindf_lex_from_buffer_options(lexer, decoded.p, cursor, decoded.len, &chars_read, options);
					cursor += chars_read;
					if (token->reg_type != PINDF_LEXER_REGTYPE_INT) {
						PINDF_ERR("failed to parse compressed object stream header, expect int, but got %d", token->reg_type);
						pindf_token_destroy(token);
						free(token);
						return NULL;
					}
					nums[j] = atoi((char*)token->raw_str->p);

					pindf_token_destroy(token);
					free(token);
				}

				PINDF_DEBUG("obj_num=%d, offset=%d", nums[0], nums[1]);

				if (nums[0] == obj_num) {
					found_flag = true;
					break;
				}
				memset(nums, 0x00, sizeof(nums));
			}
			if (!found_flag) {
				PINDF_ERR("not found obj_num=%d in this obj stream", obj_num);
				return NULL;
			}

			int offset = first + nums[1];
			pindf_pdf_obj *ret_obj = NULL;
			uint64 ret_offset = 0;
			pindf_lexer_clear(lexer);

			int ret = pindf_parse_one_obj_from_buffer(parser, lexer, &decoded, offset, &ret_obj, &ret_offset, 0);
			if (ret < 0) {
				PINDF_ERR("failed to parse obj %d from buffer", obj_num);
				if (result_of_decode == 0) {
					pindf_uchar_str_destroy(&decoded);
				}
				return NULL;
			}

			doc->ind_obj_list[obj_num] = (pindf_obj_entry){
				.available = PINDF_OBJ_AVAILABLE,
				.offset = ret_offset,
				.number = obj_num,
				.ind_obj = ret_obj,
			};

			if (result_of_decode == 0) {
				pindf_uchar_str_destroy(&decoded);
			}
		} else {
			// Free
			PINDF_WARN("obj %d is free object", obj_num);
			doc->ind_obj_list[obj_num] = (pindf_obj_entry){
				.available = PINDF_OBJ_FREE,
				.offset = 0,
				.number = 0,
				.ind_obj = NULL,
			};
		}
	}

	pindf_parser_clear(parser);
	pindf_lexer_clear(lexer);

	return doc->ind_obj_list[obj_num].ind_obj;
}