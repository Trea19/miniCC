CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99

# source files
CFILES = main.c semantics.c AST.c
LFILE = scanner.l
YFILE = parser.y
HFILES = AST.h semantics.h
LFC = lex.yy.c
YFC = parser.tab.c
OBJS = $(CFILES:.c=.o)
LFO = $(LFC:.c=.o)
YFO = $(YFC:.c=.o)

# target
miniCC: syntax $(filter-out $(LFO),$(OBJS))
	$(CC) -o miniCC $(filter-out $(LFO),$(OBJS)) -ll -ly -ggdb

syntax: lexical syntax-c
	$(CC) -c $(YFC) -o $(YFO)

lexical: $(LFILE)
	$(FLEX) -o $(LFC) $(LFILE)

syntax-c: $(YFILE)
	$(BISON) -o $(YFC) -d -v $(YFILE)

-include $(patsubst %.o, %.d, $(OBJS))


clean:
	rm -f miniCC $(LFC) $(YFC) $(YFC:.c=.h) *.o *.output 

.PHONY: all clean

