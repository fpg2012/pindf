#include "parser.h"

#define ASSERT_EMPTY_STACK() \
	do { if (parser->symbol_stack->len == 0) { perror("empty stack!"); return -1; } } while(0)
#define REDUCE_SINGLE_OBJ(type_low, type_upper, parsed_content) \
	do { \
		ASSERT_EMPTY_STACK(); \
		pindf_symbol *symbol = NULL; \
		pindf_vector_last_elem(parser->symbol_stack, &symbol); \
		pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_##type_upper); \
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

void _update_reduce_pos(pindf_parser *parser)
{
	pindf_vector_append(parser->reduce_pos_stack, &parser->reduce_pos);
	parser->reduce_pos = parser->symbol_stack->len-1;
}

void _restore_reduce_pos(pindf_parser *parser)
{
	pindf_vector_pop(parser->reduce_pos_stack, &parser->reduce_pos);
}

// reduce functions group terms to a non_term
int _reduce_array(pindf_parser *parser)
{
	size_t beg = parser->reduce_pos, p = beg;
	size_t cap = parser->symbol_stack->len - beg;
	pindf_vector *vec = pindf_vector_new(cap, sizeof(pindf_pdf_obj*), NULL);

	pindf_symbol *symbol;
	MATCH_DELIM_OR_ERR("array", '[');
	uint64 offset = symbol->offset;

	while (p < parser->symbol_stack->len - 1) {
		MATCH_OBJ_OR_ERR("array");
		pindf_vector_append(vec, &symbol->content.non_term);
	}

	MATCH_DELIM_OR_ERR("array", ']');

	POP_EVERYTHING();

	pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_ARRAY);
	obj->content.array = vec;

	symbol = pindf_symbol_new_nonterm(obj, offset);
	pindf_vector_append(parser->symbol_stack, &symbol);

	_restore_reduce_pos(parser);
	return 0;
}

int _reduce_dict(pindf_parser *parser)
{
	size_t beg = parser->reduce_pos, p = beg;
	pindf_symbol *symbol;

	size_t cap = (parser->symbol_stack->len - beg) / 2 + 1;
	pindf_vector *keys = pindf_vector_new(cap, sizeof(pindf_pdf_obj*), NULL);
	pindf_vector *values = pindf_vector_new(cap, sizeof(pindf_pdf_obj*), NULL);

	MATCH_DELIM_OR_ERR("dict", '<');
	uint64 offset = symbol->offset;

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

	pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_DICT);
	obj->content.dict.keys = keys;
	obj->content.dict.values = values;

	symbol = pindf_symbol_new_nonterm(obj, offset);
	pindf_vector_append(parser->symbol_stack, &symbol);

	_restore_reduce_pos(parser);
	return 0;
}

int _reduce_ref(pindf_parser *parser)
{
	size_t beg = parser->symbol_stack->len - 3, p = beg;
	pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_REF);
	pindf_symbol *symbol;
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

int _reduce_ind_obj(pindf_parser *parser)
{
	pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_IND_OBJ);
	pindf_symbol *symbol;
	int ab[2];

	size_t beg = parser->symbol_stack->len - 5;
	size_t p = beg;
	uint64 offset = 0;
	for (int i = 0; i < 2; ++p,++i) {
		pindf_vector_index(parser->symbol_stack, p, &symbol);
		if (symbol->type != PINDF_SYMBOL_NONTERM ||
			symbol->content.non_term->obj_type != PINDF_PDF_INT
		) {
			perror("[reduce ind obj] failed to reduce obj/gen number");
			return -1;
		}
		ab[i] = symbol->content.non_term->content.num;
		if (i == 0)
			offset = symbol->offset;
	}
	obj->content.indirect_obj.obj_num = ab[0];
	obj->content.indirect_obj.generation_num = ab[1];
	++p;

	// match an obj
	MATCH_OBJ_OR_ERR("ind obj");

	obj->content.indirect_obj.obj = symbol->content.non_term;
	++p;

	POP_EVERYTHING();

	symbol = pindf_symbol_new_nonterm(obj, offset);
	pindf_vector_append(parser->symbol_stack, &symbol);

	return 0;
}

int _reduce_int(pindf_parser *parser) { REDUCE_SINGLE_OBJ(num, PDF_INT, atoi((const char*)symbol->content.term->raw_str->p)); }
int _reduce_real(pindf_parser *parser) { REDUCE_SINGLE_OBJ(real_num, PDF_REAL, atof((const char*)symbol->content.term->raw_str->p)); }
int _reduce_null(pindf_parser *parser) { REDUCE_SINGLE_OBJ(num, PDF_NULL, 0); }
int _reduce_bool(pindf_parser *parser) { REDUCE_SINGLE_OBJ(boolean, PDF_BOOL, strcmp("true", (const char*)symbol->content.term->raw_str->p) == 0 ? 1 : 0); }
int _reduce_name(pindf_parser *parser) { REDUCE_SINGLE_OBJ(name, PDF_NAME, symbol->content.term->raw_str); }
int _reduce_ltr_str(pindf_parser *parser) { REDUCE_SINGLE_OBJ(ltr_str, PDF_LTR_STR, symbol->content.term->raw_str); }
int _reduce_hex_str(pindf_parser *parser) { REDUCE_SINGLE_OBJ(hex_str, PDF_LTR_STR, symbol->content.term->raw_str); } // maybe we should convert the raw_str here

int pindf_parser_add_stream(pindf_parser *parser, pindf_uchar_str *stream, uint64 content_offset)
{

	pindf_symbol *symbol = NULL;

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

	pindf_pdf_obj *obj = pindf_pdf_obj_new(PINDF_PDF_STREAM);
	obj->content.stream.dict = symbol->content.non_term;
	obj->content.stream.stream_content = stream;
	obj->content.stream.content_offset = content_offset;

	symbol->content.non_term = obj;
	
	return 0;
}

pindf_symbol *pindf_symbol_new_term(pindf_token *token)
{
	pindf_symbol *sym = (pindf_symbol*)malloc(sizeof(pindf_symbol));
	sym->type = PINDF_SYMBOL_TERM;
	sym->content.term = token;
	return sym;
}

pindf_symbol *pindf_symbol_new_nonterm(pindf_pdf_obj *pdf_obj, uint64 offset)
{
	pindf_symbol *sym = (pindf_symbol *)malloc(sizeof(pindf_symbol));
	sym->type = PINDF_SYMBOL_NONTERM;
	sym->content.non_term = pdf_obj;
	sym->offset = offset;
	return sym;
}

pindf_parser *pindf_parser_new()
{
	pindf_parser *parser = (pindf_parser*)malloc(sizeof(pindf_parser));
	
	parser->symbol_stack = pindf_vector_new(32768, sizeof(pindf_symbol*), NULL);
	parser->reduce_pos_stack = pindf_vector_new(1024, sizeof(int), NULL);

	// parser->file_part_state = 0;

	parser->reduce_pos = 0;

	return parser;
}

void pindf_parser_init(pindf_parser *parser)
{
	parser->reduce_pos = 0;
	parser->symbol_stack->len = 0;
	parser->reduce_pos_stack->len = 0;
}

int pindf_parser_add_token(pindf_parser *parser, pindf_token *token)
{
	assert(token != NULL);

	// append token
	pindf_symbol *symbol = pindf_symbol_new_term(token);
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
			fprintf(stderr, "[error] unrecognized regular token %s", token->raw_str->p);
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

void pindf_parser_destroy(void *parser)
{
	pindf_parser *parser_ = (pindf_parser *)parser;
	pindf_vector_destroy(parser_->symbol_stack);
	pindf_vector_destroy(parser_->reduce_pos_stack);
}