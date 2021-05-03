all: lexer.fl parser.y
	bison -d parser.y
	flex lexer.fl
	gcc parser.tab.c lex.yy.c command.c -ll -lm -g -o shell
clean:
	rm -f parser.h
	rm -f lex.yy.c
	rm -f parser.tab.c
	rm -f parser.tab.h
	rm -f *.o
	rm -f op.h
	rm -f op.c
	rm -f a.out
	rm -f shell
	rm -f lexer.c
	rm -f *~