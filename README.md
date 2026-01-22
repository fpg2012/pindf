# pindf

Simple pdf parser

## try pindf

Clone this repo, and build with `make`.

```
cd pindf
make
```

### lexer

Find some pdf, and run the following command to print the result of lexical analysis.

```
./lexer_test [your_pdf_file]
```

### parser

Convert the pdf structure to a json file.

```
./parser_test [your_pdf_file]
```
