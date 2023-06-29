CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99

# 源文件列表
CFILES = main.c semantics.c AST.c
LFILE = scanner.l
YFILE = parser.y
HFILES = AST.h semantics.h
LFC = lex.yy.c
YFC = parser.tab.c
OBJS = $(CFILES:.c=.o)

# 默认目标
all: miniCC

# 编译目标
miniCC: $(OBJS) $(YFC) $(LFC)
	$(CC) $(CFLAGS) -o miniCC $(OBJS) $(YFC) $(LFC) -lfl

# 词法分析器生成规则
$(LFC): $(LFILE)
	$(FLEX) -o $(LFC) $(LFILE)

# 语法分析器生成规则
$(YFC): $(YFILE)
	$(BISON) -o $(YFC) -d $(YFILE)

# 目标文件生成规则
%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f miniCC $(LFC) $(YFC) $(OBJS)

.PHONY: all clean