all:
	CC main.c lexer.c uchar_str.c -g
run: all
	./a.out
clean:
	rm a.out lexer.h.pch
	rm -r a.out.dSYM