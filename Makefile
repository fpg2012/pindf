SRC = pdf/*.c container/*.c 
SRC += core/lexer.c core/parser.c core/serialize.c
SRC += file_parse.c
CFLAGS += -g -std=c11

all: lexer_test parser_test vec_test
lexer_test: lexer_test.c ${SRC}
	CC -o lexer_test lexer_test.c ${SRC} ${CFLAGS}
parser_test: parser_test.c ${SRC}
	CC -o parser_test parser_test.c ${SRC} ${CFLAGS}
filter_test: count_filter_type.c ${SRC}
	CC -o count_filter_type count_filter_type.c ${SRC} ${CFLAGS}
vec_test: vec_test.c container/simple_vector.c
	CC -o vec_test vec_test.c container/simple_vector.c ${CFLAGS}
clean:
	rm -f *_test
	rm -f *.pch
	rm -rf *.dSYM