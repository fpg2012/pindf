BUILD_TYPE ?= DEBUG
BUILD_TYPE_LOWER := $(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]')

SRC = pdf/*.c container/*.c 
SRC += core/lexer.c core/parser.c core/serialize.c stream/*.c
SRC += pindf.c
SRC += logger/logger.c
CFLAGS += -std=c11

ZLIB_CFLAGS := $(shell pkg-config --cflags zlib 2>/dev/null || echo "-I/usr/local/include")
ZLIB_LDFLAGS := $(shell pkg-config --libs zlib 2>/dev/null || echo "-lz -L/usr/local/lib")
CFLAGS += $(ZLIB_CFLAGS)
LDFLAGS += $(ZLIB_LDFLAGS)

ifeq ($(BUILD_TYPE_LOWER),debug)
    CFLAGS += -g -Wall
endif
ifeq ($(BUILD_TYPE_LOWER),release)
    CFLAGS += -O2 -Wall -DNDEBUG
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    OS := Linux
    DYLIB_NAME = libpindf.so
endif
ifeq ($(UNAME_S),Darwin)
    OS := macOS
    DYLIB_NAME = libpindf.dylib
endif
ifeq ($(UNAME_S),Windows_NT)
    OS := Windows
    DYLIB_NAME = libpindf.dll
endif

# CFLAGS += -fsanitize=address

all: dylib

# compile to dynamic library base on OS
dylib:
	CC -shared -fPIC -o $(DYLIB_NAME) $(SRC) $(CFLAGS) $(LDFLAGS)

test: lexer_test parser_test vec_test modif_test dict_test doc_save_modif_test

doc: Doxyfile
	mkdir -p docs/doxygen
	doxygen Doxyfile
	cd docs && sphinx-build -b html . _build/html

lexer_test: test/lexer_test.c $(SRC)
	CC -o test/lexer_test test/lexer_test.c $(SRC) $(CFLAGS) $(LDFLAGS)

parser_test: test/parser_test.c $(SRC)
	CC -o test/parser_test test/parser_test.c $(SRC) $(CFLAGS) $(LDFLAGS)
	CC -o test/parser_from_buffer_test test/parser_from_buffer_test.c $(SRC) $(CFLAGS) $(LDFLAGS)

vec_test: test/vec_test.c container/simple_vector.c
	CC -o test/vec_test test/vec_test.c container/simple_vector.c $(CFLAGS)

modif_test: test/modif_test.c $(SRC)
	CC -o test/modif_test test/modif_test.c $(SRC) $(CFLAGS) $(LDFLAGS)

compress_test: test/compress_test.c $(SRC)
	CC -o test/compress_test test/compress_test.c $(SRC) $(CFLAGS) $(LDFLAGS)

dict_test: test/dict_test.c $(SRC)
	CC -o test/dict_test test/dict_test.c $(SRC) $(CFLAGS) $(LDFLAGS)

doc_save_modif_test: test/doc_save_modif_test.c $(SRC)
	CC -o test/doc_save_modif_test test/doc_save_modif_test.c $(SRC) $(CFLAGS) $(LDFLAGS)

clean_doc:
	rm -rf docs/_build
	rm -rf docs/doxygen

clean: clean_doc
	rm -f test/*_test
	rm -f test/*.pch
	rm -rf test/*.dSYM
	rm -rf *.dSYM
	rm -f $(DYLIB_NAME)

.PHONY: all dylib test clean clean_doc