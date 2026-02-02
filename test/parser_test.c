#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pindf.h"
#include "../logger/logger.h"


int main(int argc, const char **argv)
{
	if (argc != 2) {
		perror("Too more or too few arguments");
		fprintf(stderr, "Usage:\n\t %s [filename]", argv[0]);
		exit(0);
	}
	pindf_set_log_level(PINDF_LOG_LEVEL_DEBUG);
	pindf_lexer *lexer = pindf_lexer_new();
	pindf_parser *parser = pindf_parser_new();

	FILE *f = fopen(argv[1], "r");

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
	
	pindf_parser_clear(parser);
	pindf_lexer_clear(lexer);

	// === dump xref_table ===
	printf("\n=== dump xref table ===");
	FILE *out = fopen("xref.txt", "wb");

	int n_sections = doc->xref->n_sections;

	pindf_xref_table *section = doc->xref->first_section;
	int section_index = 0;
	while (section != NULL) {
		pindf_xref_entry *entry = NULL;
		fprintf(out, "\n--- xref section %d (st=%zu len=%zu) ---\n", section_index, section->obj_num, section->len);
		for (uint j = 0; j < section->len; ++j) {
			entry = pindf_xref_table_getentry(section, j);
			fprintf(out, "%d %06llu %llu\n", entry->type, entry->fields[0], entry->fields[1]);
		}

		section = section->next_table;
		++section_index;
	}
	fclose(out);

	// === dump ind objs ===
	out = fopen("json_dump.txt", "wb");
	pindf_uchar_str buf;
	pindf_uchar_str_init(&buf, 10000);
	fprintf(out, "[\n");
	for (int i = 0; i < doc->xref->size; ++i) {
		if (i > 0) {
			fprintf(out, ",\n");
		}
		pindf_pdf_obj *obj = pindf_doc_getobj(doc, parser, lexer, i);
		if (obj != NULL) {
			pindf_pdf_obj_serialize_json(obj, (char*)buf.p, buf.capacity);
			fprintf(out, "%s", buf.p);
		} else {
			fprintf(out, "null");
		}
	}
	fprintf(out, "]\n");
	fclose(out);
	
	return 0;
}