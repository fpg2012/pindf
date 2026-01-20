SRC_FILES = lexer.c uchar_str.c simple_vector.c pdf_obj.c pdf_doc.c
CFLAGS += -g

all: lexer_test parser_test vec_test
lexer_test: lexer_test.c ${SRC_FILES}
	CC -o lexer_test lexer_test.c ${SRC_FILES} ${CFLAGS}
parser_test: parser_test.c parser.c ${SRC_FILES}
	CC -o parser_test parser_test.c parser.c ${SRC_FILES} ${CFLAGS}
vec_test: vec_test.c simple_vector.c
	CC -o vec_test vec_test.c simple_vector.c ${CFLAGS}
clean:
	rm -f *_test
	rm -f *.pch
	rm -rf *.dSYM