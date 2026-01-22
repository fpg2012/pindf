SRC = pdf/*.c container/*.c 
SRC += core/lexer.c core/parser.c core/serialize.c
SRC += pindf.c
CFLAGS += -g -std=c11

ZLIB_CFLAGS := $(shell pkg-config --cflags zlib 2>/dev/null || echo "-I/usr/local/include")
ZLIB_LDFLAGS := $(shell pkg-config --libs zlib 2>/dev/null || echo "-lz -L/usr/local/lib")
CFLAGS += $(ZLIB_CFLAGS)
LDFLAGS += $(ZLIB_LDFLAGS)

all: lexer_test parser_test vec_test filter_test

lexer_test: test/lexer_test.c ${SRC}
	CC -o test/lexer_test test/lexer_test.c ${SRC} ${CFLAGS}

parser_test: test/parser_test.c ${SRC}
	CC -o test/parser_test test/parser_test.c ${SRC} ${CFLAGS}

filter_test: test/filter_test.c ${SRC}
	CC -o test/filter_test test/filter_test.c ${SRC} ${CFLAGS} ${LDFLAGS}

vec_test: test/vec_test.c container/simple_vector.c
	CC -o test/vec_test test/vec_test.c container/simple_vector.c ${CFLAGS}

clean:
	rm -f test/*_test
	rm -f test/*.pch
	rm -rf test/*.dSYM