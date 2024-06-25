kconf:
	bison -d grammer.y
	flex scan.l
	gcc lex.yy.c grammer.tab.c -lfl -o kconf

clean:
	rm grammer.tab.*
	rm lex.yy.c
	rm kconf
