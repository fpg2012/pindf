SRC = pdf/*.c container/*.c 
SRC += core/lexer.c core/parser.c core/serialize.c stream/*.c
SRC += pindf.c
SRC += logger/logger.c
CFLAGS += -g -std=c11

ZLIB_CFLAGS := $(shell pkg-config --cflags zlib 2>/dev/null || echo "-I/usr/local/include")
ZLIB_LDFLAGS := $(shell pkg-config --libs zlib 2>/dev/null || echo "-lz -L/usr/local/lib")
CFLAGS += $(ZLIB_CFLAGS)
LDFLAGS += $(ZLIB_LDFLAGS)

# CFLAGS += -fsanitize=address

all: lexer_test parser_test vec_test

doc: Doxyfile
	mkdir -p docs/doxygen
	doxygen Doxyfile
	cd docs && sphinx-build -b html . _build/html

lexer_test: test/lexer_test.c ${SRC}
	CC -o test/lexer_test test/lexer_test.c ${SRC} ${CFLAGS} ${LDFLAGS}

parser_test: test/parser_test.c ${SRC}
	CC -o test/parser_test test/parser_test.c ${SRC} ${CFLAGS} ${LDFLAGS}
	CC -o test/parser_from_buffer_test test/parser_from_buffer_test.c ${SRC} ${CFLAGS} ${LDFLAGS}

vec_test: test/vec_test.c container/simple_vector.c
	CC -o test/vec_test test/vec_test.c container/simple_vector.c ${CFLAGS}

compress_test: test/compress_test.c ${SRC}
	CC -o test/compress_test test/compress_test.c ${SRC} ${CFLAGS} ${LDFLAGS}

clean_doc:
	rm -rf docs/_build
	rm -rf docs/doxygen

clean: clean_doc
	rm -f test/*_test
	rm -f test/*.pch
	rm -rf test/*.dSYM