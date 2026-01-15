all:
	CC main.c lexer.c lexer.h -g
run:
	./main
clean:
	rm main