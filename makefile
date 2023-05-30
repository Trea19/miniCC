CC = gcc
OUT = miniCC
SCANNER = scanner.l
PARSER = parse.y

build: $(OUT)

clean:
	rm -f *.o lex.yy.c parse.tab.c parse.tab.h parse.output $(OUT)

$(OUT): lex.yy.c main.c ir.c symbol_table.c syntax_tree.c parse.tab.c compiler.h
	$(CC) -o $(OUT) lex.yy.c main.c ir.c symbol_table.c syntax_tree.c parse.tab.c
 
lex.yy.c: $(SCANNER) parse.tab.c 
	flex $<

parse.tab.c: $(PARSER)
	bison -d $<
