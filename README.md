# libpindf

Simple pdf parser library.

## Build

Clone this repo, and build with `make`.

```
cd pindf
make
```

`libpindf.so` / `libpindf.dylib` / `libpindf.dll` will be generated. To build release version, use `make BUILD_TYPE=release`.

To build doxygen documentation, use `make doc`.

## Examples

Examples are in the `test` directory. To build tests, use `make test`.

### Lexer

Find some pdf, and run the following command to print the result of lexical analysis.

```
./test/lexer_test [your_pdf_file]
```

### Parser

Convert the pdf structure to a json file.

```
./test/parser_test [your_pdf_file]
```

### Outline modification test

Append a new outline to the pdf file.

```
./test/doc_save_modif_test [your_pdf_file] [output_pdf_file]
```
