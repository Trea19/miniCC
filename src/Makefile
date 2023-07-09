CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99

CFILES = $(shell find ./ -name "*.c")
OBJS = $(CFILES:.c=.o)
TESTS = $(shell find ./../Test -name "sem*.cmm");
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")
LFC = $(shell find ./ -name "*.l" | sed s/[^/]*\\.l/lex.yy.c/)
YFC = $(shell find ./ -name "*.y" | sed s/[^/]*\\.y/syntax.tab.c/)
LFO = $(LFC:.c=.o)
YFO = $(YFC:.c=.o)

miniCC: syntax $(filter-out $(LFO),$(OBJS))
    $(CC) -o ../miniCC $(filter-out $(LFO),$(OBJS)) -ll -ly -ggdb

syntax: lexical syntax-c
    $(CC) -c $(YFC) -o $(YFO) -ggdb

lexical: $(LFILE)
    $(FLEX) -o $(LFC) $(LFILE)

syntax-c: $(YFILE)
    $(BISON) -o $(YFC) -d -v $(YFILE)

-include $(patsubst %.o, %.d, $(OBJS))


.PHONY: clean 

clean:
    rm -f lex.yy.c syntax.tab.c syntax.tab.h syntax.output
    rm -f $(OBJS) $(OBJS:.o=.d)
    rm -f $(LFC) $(YFC) $(YFC:.c=.h)

