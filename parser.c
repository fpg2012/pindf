#include "parser.h"
#include "lexer.h"
#include "pdf_doc.h"
#include "pdf_obj.h"
#include "simple_vector.h"
#include "uchar_str.h"
#include <stdio.h>
#include <stdlib.h>

#define ASSERT_EMPTY_STACK() \
	do { if (parser->symbol_stack->len == 0) { perror("empty stack!"); return -1; } } while(0)
#define REDUCE_SINGLE_OBJ(type_low, type_upper, parsed_content) \
	do { \
		ASSERT_EMPTY_STACK(); \
		struct pindf_symbol *symbol = NULL; \
		pindf_vector_last_elem(parser->symbol_stack, &symbol); \
		struct pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_##type_upper); \
		obj->content.type_low = (parsed_content); \
		symbol->type = PINDF_SYMBOL_NONTERM; \
		symbol->content.non_term = obj; \
		return 0; \
	} while(0)
#define MATCH_DELIM_OR_ERR(reduce_type, delim) \
	do { \
		pindf_vector_index(parser->symbol_stack, p, &symbol); \
		if (!(symbol->type == PINDF_SYMBOL_TERM &&  \
			symbol->content.term->event == PINDF_LEXER_EMIT_DELIM && \
			symbol->content.term->raw_str->p[0] == delim) \
		) { \
			fprintf(stderr, "[reduce %s] no delim %c found", reduce_type, delim); \
			return -1; \
		} \
	++p; } while(0)
#define MATCH_OBJ_OR_ERR(reduce_type) \
	do { \
		pindf_vector_index(parser->symbol_stack, p, &symbol); \
		if (symbol->type != PINDF_SYMBOL_NONTERM) { \
			fprintf(stderr, "[reduce %s] invalid obj", reduce_type); \
			return -1; \
		} \
		++p; \
	} while (0)
#define FULL_REDUCE_CHECK(reduce_type) \
	do { \
		if (p != parser->symbol_stack->len) { \
			fprintf(stderr, "[reduce %s] there is something remain in stack ", reduce_type); \
			return -1; \
		} \
	} while (0)
#define POP_EVERYTHING() \
	do { \
		while (p > beg) { \
			pindf_vector_pop(parser->symbol_stack, NULL); \
			--p; \
		} \
	} while (0)


void _update_reduce_pos(struct pindf_parser *parser)
{
	pindf_vector_append(parser->reduce_pos_stack, &parser->last_reduct_pos);
	parser->last_reduct_pos = parser->symbol_stack->len-1;
}

void _restore_reduce_pos(struct pindf_parser *parser)
{
	pindf_vector_pop(parser->reduce_pos_stack, &parser->last_reduct_pos);
}

// reduce functions group terms to a non_term
int _reduce_array(struct pindf_parser *parser)
{
	size_t beg = parser->last_reduct_pos, p = beg;
	size_t cap = parser->symbol_stack->len - beg;
	struct pindf_vector *vec = pindf_vector_new(cap, sizeof(struct pindf_pdf_obj*), NULL);

	struct pindf_symbol *symbol;
	MATCH_DELIM_OR_ERR("array", '[');

	while (p < parser->symbol_stack->len - 1) {
		MATCH_OBJ_OR_ERR("array");
		pindf_vector_append(vec, &symbol->content.non_term);
	}

	MATCH_DELIM_OR_ERR("array", ']');

	POP_EVERYTHING();

	struct pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_ARRAY);
	obj->content.array.items = vec;

	symbol = pindf_symbol_new_nonterm(obj);
	pindf_vector_append(parser->symbol_stack, &symbol);

	_restore_reduce_pos(parser);
	return 0;
}

int _reduce_dict(struct pindf_parser *parser)
{
	size_t beg = parser->last_reduct_pos, p = beg;
	struct pindf_symbol *symbol;

	size_t cap = (parser->symbol_stack->len - beg) / 2 + 1;
	struct pindf_vector *keys = pindf_vector_new(cap, sizeof(struct pindf_pdf_obj*), NULL);
	struct pindf_vector *values = pindf_vector_new(cap, sizeof(struct pindf_pdf_obj*), NULL);

	MATCH_DELIM_OR_ERR("dict", '<');

	while (p < parser->symbol_stack->len - 1) {
		MATCH_OBJ_OR_ERR("dict");
		if (symbol->content.non_term->obj_type != PINDF_PDF_NAME) {
			perror("[reduce dict] non-name key is invalid");
			return -1;
		}
		pindf_vector_append(keys, &symbol->content.non_term);

		MATCH_OBJ_OR_ERR("dict");
		pindf_vector_append(values, &symbol->content.non_term);
	}

	MATCH_DELIM_OR_ERR("dict", '>');

	POP_EVERYTHING();

	struct pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_DICT);
	obj->content.dict.keys = keys;
	obj->content.dict.values = values;

	symbol = pindf_symbol_new_nonterm(obj);
	pindf_vector_append(parser->symbol_stack, &symbol);

	_restore_reduce_pos(parser);
	return 0;
}

int _reduce_ref(struct pindf_parser *parser)
{
	size_t beg = parser->symbol_stack->len - 3, p = beg;
	struct pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_REF);
	struct pindf_symbol *symbol;
	int ab[2];
	for (int i = 0; i < 2; ++i, ++p) {
		pindf_vector_index(parser->symbol_stack, p, &symbol);
		ab[i] = symbol->content.non_term->content.num;
	}
	pindf_vector_pop(parser->symbol_stack, NULL); // pop R
	pindf_vector_pop(parser->symbol_stack, NULL); // pop gen num

	pindf_vector_last_elem(parser->symbol_stack, &symbol);
	obj->content.ref.obj_num = ab[0];
	obj->content.ref.generation_num = ab[1];
	
	// potential memory leak
	symbol->type = PINDF_SYMBOL_NONTERM;
	symbol->content.non_term = obj;
	return 0;
}

int _reduce_ind_obj(struct pindf_parser *parser)
{
	struct pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_IND_OBJ);
	struct pindf_symbol *symbol;
	int ab[2];

	size_t beg = parser->symbol_stack->len - 5;
	size_t p = beg;
	for (int i = 0; i < 2; ++p,++i) {
		pindf_vector_index(parser->symbol_stack, p, &symbol);
		if (symbol->type != PINDF_SYMBOL_NONTERM ||
			symbol->content.non_term->obj_type != PINDF_PDF_INT
		) {
			perror("[reduce ind obj] failed to reduce obj/gen number");
			return -1;
		}
		ab[i] = symbol->content.non_term->content.num;
	}
	obj->content.indirect_obj.obj_num = ab[0];
	obj->content.indirect_obj.generation_num = ab[1];
	++p;

	// match an obj
	MATCH_OBJ_OR_ERR("ind obj");

	obj->content.indirect_obj.obj = symbol->content.non_term;
	++p;

	POP_EVERYTHING();

	symbol = pindf_symbol_new_nonterm(obj);
	pindf_vector_append(parser->symbol_stack, &symbol);

	return 0;
}

int _reduce_int(struct pindf_parser *parser) { REDUCE_SINGLE_OBJ(num, PDF_INT, atoi((const char*)symbol->content.term->raw_str->p)); }
int _reduce_real(struct pindf_parser *parser) { REDUCE_SINGLE_OBJ(real_num, PDF_REAL, atof((const char*)symbol->content.term->raw_str->p)); }
int _reduce_null(struct pindf_parser *parser) { REDUCE_SINGLE_OBJ(num, PDF_NULL, 0); }
int _reduce_bool(struct pindf_parser *parser) { REDUCE_SINGLE_OBJ(boolean, PDF_BOOL, strcmp("true", (const char*)symbol->content.term->raw_str->p) == 0 ? 1 : 0); }
int _reduce_name(struct pindf_parser *parser) { REDUCE_SINGLE_OBJ(name, PDF_NAME, symbol->content.term->raw_str); }
int _reduce_ltr_str(struct pindf_parser *parser) { REDUCE_SINGLE_OBJ(ltr_str, PDF_LTR_STR, symbol->content.term->raw_str); }
int _reduce_hex_str(struct pindf_parser *parser) { REDUCE_SINGLE_OBJ(hex_str, PDF_LTR_STR, symbol->content.term->raw_str); } // maybe we should convert the raw_str here

int pindf_parser_add_stream(struct pindf_parser *parser, struct pindf_uchar_str *stream)
{

	struct pindf_symbol *symbol = NULL;

	if (parser->symbol_stack->len == 0) {
		perror("stream dict not found!");
		return -1;
	}

	pindf_vector_last_elem(parser->symbol_stack, &symbol);
	if (symbol->type != PINDF_SYMBOL_NONTERM
		&& symbol->content.non_term->obj_type != PINDF_PDF_DICT
	) {
		perror("stream dict not found!");
		return -1;
	}

	struct pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_STREAM);
	obj->content.stream.dict = symbol->content.non_term;
	obj->content.stream.stream_content = stream;

	symbol->content.non_term = obj;
	
	return 0;
}

struct pindf_symbol *pindf_symbol_new_term(struct pindf_token *token)
{
	struct pindf_symbol *sym = (struct pindf_symbol*)malloc(sizeof(struct pindf_symbol));
	sym->type = PINDF_SYMBOL_TERM;
	sym->content.term = token;
	return sym;
}

struct pindf_symbol *pindf_symbol_new_nonterm(struct pindf_pdf_obj *pdf_obj)
{
	struct pindf_symbol *sym = (struct pindf_symbol *)malloc(sizeof(struct pindf_symbol));
	sym->type = PINDF_SYMBOL_NONTERM;
	sym->content.non_term = pdf_obj;
	return sym;
}

struct pindf_parser *pindf_parser_new()
{
	struct pindf_parser *parser = (struct pindf_parser*)malloc(sizeof(struct pindf_parser));
	
	parser->symbol_stack = pindf_vector_new(32768, sizeof(struct pindf_symbol*), NULL);
	parser->dict_state_stack = pindf_vector_new(1024, sizeof(int), NULL);
	parser->reduce_pos_stack = pindf_vector_new(1024, sizeof(int), NULL);

	parser->dict_level = 0;
	parser->array_level = 0;

	// parser->file_part_state = 0;

	parser->last_reduct_pos = 0;

	return parser;
}

int pindf_parser_add_token(struct pindf_parser *parser, struct pindf_token *token)
{
	assert(token != NULL);

	// append token
	struct pindf_symbol *symbol = pindf_symbol_new_term(token);
	pindf_vector_append(parser->symbol_stack, &symbol);
 
	int ret = 0;

	switch (token->event) {
	case PINDF_LEXER_EMIT_REGULAR:
		if (token->reg_type == PINDF_LEXER_REGTYPE_INT) {
			ret = _reduce_int(parser);
		} else if (token->reg_type == PINDF_LEXER_REGTYPE_REAL) {
			ret = _reduce_real(parser);
		} else if (token->reg_type == PINDF_LEXER_REGTYPE_KWD) {
			if (token->kwd == PINDF_KWD_R) {
				ret = _reduce_ref(parser);
			} else if (token->kwd == PINDF_KWD_obj) {
				; // do nothing
			} else if (token->kwd == PINDF_KWD_endobj) {
				ret = _reduce_ind_obj(parser);
			} else if (token->kwd == PINDF_KWD_null) {
				ret = _reduce_null(parser);
			} else if (token->kwd == PINDF_KWD_true || token->kwd == PINDF_KWD_false) {
				ret = _reduce_bool(parser);
			} else if (token->kwd == PINDF_KWD_xref) {
				return -2;
			} else if (token->kwd == PINDF_KWD_trailer) {
				return -2;
			} else if (token->kwd == PINDF_KWD_startxref) {
				return -2;
			} else if (token->kwd == PINDF_KWD_stream || token->kwd == PINDF_KWD_endstream) {
				pindf_vector_pop(parser->symbol_stack, NULL);
			} else {
				fprintf(stderr, "invalid usage of keyword %s", token->raw_str->p);
			}
		} else {
			perror("unrecognized regular token");
			return -1;
		}
		break;
	case PINDF_LEXER_EMIT_DELIM:
		if (token->raw_str->p[0] == '<') {
			_update_reduce_pos(parser);
		} else if (token->raw_str->p[0] == '>') {
			ret = _reduce_dict(parser);
		} else if (token->raw_str->p[0] == '[') {
			_update_reduce_pos(parser);
		} else if (token->raw_str->p[0] == ']') {
			ret = _reduce_array(parser);
		} else {
			perror("invalid delim:");
			perror((const char*)token->raw_str->p);
			return -1;
		}
		break;
	case PINDF_LEXER_EMIT_LTR_STR:
		ret = _reduce_ltr_str(parser);
		break;
	case PINDF_LEXER_EMIT_HEX_STR:
		ret = _reduce_hex_str(parser);
		break;
	case PINDF_LEXER_EMIT_NAME:
		ret = _reduce_name(parser);
		break;
	case PINDF_LEXER_EMIT_COMMENT:
		break;
	case PINDF_LEXER_EMIT_WHITE_SPACE:
	case PINDF_LEXER_EMIT_EOL:
	case PINDF_LEXER_NO_EMIT:
		pindf_vector_pop(parser->symbol_stack, NULL);
		break;
	case PINDF_LEXER_EMIT_ERR:
	case PINDF_LEXER_EMIT_EOF:
	default:
		perror("invalid event");
	}

	return ret;
}

void pindf_parser_xref(void *parser, FILE *fp);

uint64 _quick_match_startxref(FILE *fp, uint64 file_len_ptr)
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

void pindf_parser_file_parse(struct pindf_parser *parser, FILE *fp, uint64 file_len)
{
	// 0 - HEADER
	// 1 - BODY
	// 2 - XREF
	// 3 - trailer
	// 4 - end
	int file_part_state = 0;

	struct pindf_lexer *lexer = pindf_lexer_new();

	int stream_state = 0;
	int stream_len = 0;

	int ret = -1;

	uint64 xref_start;

	struct pindf_token *token = NULL;

	struct pindf_pdf_doc *doc = pindf_pdf_doc_new("PDF-1.7");

	// first pass: quick read version and xref and trailer

	// version
	token = pindf_lex(lexer, fp);
	if (token->event == PINDF_LEXER_EMIT_COMMENT) {
		char *p = (char*)calloc(1, token->raw_str->len + 10);
		memcpy(p, (char*)token->raw_str->p, token->raw_str->len);
		p[token->raw_str->len] = '\0';
		doc->pdf_version = p;
	} else {
		fprintf(stderr, "PDF Version not found! defaulted to %s", "PDF-1.7");
	}

	// startxref
	uint64 startxref_start = _quick_match_startxref(fp, file_len);

	while (1) {
		token = pindf_lex(lexer, fp);
		if (token->event == PINDF_LEXER_EMIT_REGULAR &&
			token->reg_type == PINDF_LEXER_REGTYPE_INT
		) {
		xref_start = atoll((const char*)token->raw_str->p);
			break;
		} else if (token->event == PINDF_LEXER_EMIT_COMMENT ||
			token->event == PINDF_LEXER_NO_EMIT ||
			token->event == PINDF_LEXER_EMIT_WHITE_SPACE ||
			token->event == PINDF_LEXER_EMIT_EOL
		) {
			continue;
		} else {
			perror("Invalid startref!");
			exit(0);
		}
	}

	printf("startref: %llu\n", xref_start);
}

void pindf_parser_destroy(void *parser)
{
	struct pindf_parser *parser_ = (struct pindf_parser *)parser;
	pindf_vector_destroy(parser_->symbol_stack);
	pindf_vector_destroy(parser_->dict_state_stack);
	pindf_vector_destroy(parser_->reduce_pos_stack);
}