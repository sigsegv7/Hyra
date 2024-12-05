kconf:
	bison -d grammar.y
	flex scan.l
	gcc lex.yy.c grammar.tab.c -lfl -o kconf

clean:
	rm grammar.tab.*
	rm lex.yy.c
	rm kconf
