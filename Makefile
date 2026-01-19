SRC_FILES = lexer.c uchar_str.c simple_vector.c pdf_obj.c

all: lexer_test parser_test vec_test
lexer_test:
	CC -o lexer_test lexer_test.c ${SRC_FILES} -g
parser_test:
	CC -o parser_test parser_test.c parser.c ${SRC_FILES} -g
vec_test:
	CC -o vec_test vec_test.c simple_vector.c
clean:
	rm -f *_test
	rm -f *.pch
	rm -rf *.dSYM