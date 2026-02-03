#include <stdio.h>
#include <stdlib.h>
#include "../pindf.h"

const char *input = "<</Type /ObjStm /Hey 0 /jump << /foo /bar /gg 3.9 >> /arr [2 3 4 4 0.2 null]>>";

int main()
{
	pindf_parser *parser = pindf_parser_new();
	pindf_lexer *lexer = pindf_lexer_new();
	pindf_lexer_init(lexer);

	pindf_pdf_obj *obj = NULL;
	uint64_t ret_offset = 0;
	pindf_uchar_str *buf = pindf_uchar_str_from_cstr(input, strlen(input));
	int ret = pindf_parse_one_obj_from_buffer(parser, lexer, buf, 0, &obj, &ret_offset, PINDF_PDF_UNK);

	char *buf2 = (char*)calloc(10000, sizeof(char));
	pindf_pdf_obj_serialize_json(obj, buf2, 10000);
	printf("%s", buf2);
}