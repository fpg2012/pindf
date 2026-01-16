all:
	CC -o lexer_test lexer_test.c lexer.c uchar_str.c -g
clean:
	rm lexer_test
	rm *.pch || rm -r *.dSYM