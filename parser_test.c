#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/parser.h"
#include "core/lexer.h"
#include "core/serialize.h"
#include "pdf/doc.h"
#include "pdf/obj.h"
#include "container/simple_vector.h"
#include "container/uchar_str.h"
#include "file_parse.h"


int main(int argc, const char **argv)
{
	if (argc != 2) {
		perror("Too more or too few arguments");
		fprintf(stderr, "Usage:\n\t %s [filename]", argv[0]);
		exit(0);
	}
	pindf_lexer *lexer = pindf_lexer_new();
	pindf_parser *parser = pindf_parser_new();

	FILE *f = fopen(argv[1], "r");

	int stream_state = 0;
	int stream_len = 0;

	int ret = -1;

	pindf_token *token = NULL;
	pindf_doc *doc = NULL;

	uint64 file_len = 0;

	printf("\n=== file len ===\n");
	fseek(f, 0, SEEK_END);
	file_len = ftell(f);
	printf("%llu\n", file_len);

	assert(file_len > 0);

	// === quick file parse test ===
	printf("\n=== quick file parse test ===\n");
	fseek(f, 0, SEEK_SET);
	ret = pindf_file_parse(parser, f, file_len, &doc);
	if (ret < 0) {
		return -1;
	}
	printf("startxref: %d\n", doc->xref_offset);
	
	pindf_parser_init(parser);
	pindf_lexer_init(lexer);
	// === body parse test ===
	printf("\n=== body parse test ===\n");
	fseek(f, 0, SEEK_SET);
	uint64 obj_offset;
	lexer->offset = 0;
	
	int obj_count = 0;
	while (1) {
		pindf_pdf_obj *obj = NULL;
		ret = pindf_parse_one_obj(parser, lexer, f, &obj, &obj_offset, PINDF_PDF_IND_OBJ);
		if (ret == -3) {
			break;
		}
		if (ret < 0) {
			fprintf(stderr, "[error] failed to parser one obj, offset=%llu", obj_offset);
		}
		
		
		++obj_count;
		pindf_doc_obj_setentry(doc, obj, obj_offset);
		
		pindf_vector_pop(parser->symbol_stack, NULL);
	}

	printf("obj_count: %d\n", obj_count);

	// === serialize to json ===
	printf("\n=== serialize to json ===\n");
	char *buf = (char*)malloc(file_len * 2);

	char *p = pindf_doc_serialize_json(doc, buf, file_len*2);
	printf("buf len: %ld", (p - buf));

	FILE *out = fopen("out.json", "wb");
	// fwrite(buf, sizeof(char), p - buf, stdout);
	fwrite(buf, sizeof(char), p - buf, out);
	
	return 0;
}