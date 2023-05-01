SCANNER = scanner.l
PARSER = parse.y
OUT = tcc
OBJ = y.tab.c lex.yy.c

$(OUT): $(OBJ)
	gcc -o $(OUT) $(OBJ)

lex.yy.c: $(SCANNER)
	flex $<

y.tab.c: $(PARSER)
	bison -vdty $<
