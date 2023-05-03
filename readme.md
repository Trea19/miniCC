a)可过滤 \t \a \r 产生的空格
b)注释 支持使用 // 或 # 进行单行注释
c)运算符 支持 +, *, -, /, %算术运算符；支持 >, <, >=, <=, !=, == 比较运算符；支持 &&, ||, ! 逻辑运算符。运算符优先级从低到高（同一行优先级相同，从左向右结合）：
 - =
 - ||
 - &&
 - ==  !=
 - <  >  <=  >=
 - +  - (双目，减号)
 - *  /  %
 - !  - (单目，符号)
d)界符 支持 ,  ;  (  )  {  } 
e)函数 
 - 函数返回类型为int或void，可传入参数（空或形如“int a, int b, ...”）
 - 函数名称规定以字母或下划线开头，可包含字母、下划线、数字
 - 函数中可在函数体内的开头声明int类型变量（声明多个变量的格式形如），先声明，后赋值，赋值跟声明不能同时进行
 - 支持 = 连接的赋值语句
 - 支持 return 返回语句，可返回int、void类型，也支持形如“return ;”
 - 支持 while、break、continue 循环语句
 - 支持 if和if - else 条件语句，判断语句需用括号扩起，形如“if (...)”
 - 支持 readint 读入语句
 - 支持 print 输出语句，可输出字符串常量、
 - 支持函数嵌套
 - 支持int、void、字符串类型变量
f)标识符规定以字母或下划线开头，可包含字母、下划线、数字
g)函数中使用变量时需要先声明，后赋值，赋值跟声明不能同时进行
h)错误提示包括：不可识别字符、单行未完结字符串、其他（行号+msg）

以下用BNF文法表示语法规则：
```
<Program> ::= ε | < Function Definition> <Program>

<Function Definition> ::= <Return Type> <Function Name> "("<Arguments>")" "{" <Variable Declarations> <Statements> "}"

<Return Type> ::= "int" | "void" 

<Function Name> ::= <Identifier>

<Arguments> ::=  ε | <_Arguments>

<_Arguments> ::=  “int” <Identifier> | “int” <Identifier> “,” <_Arguments>

<Variable Declarations> ::=  ε | <Variable Declaration> <Variable Declarations> “;”

<Variable Declaration> ::=  “int” <Identifier> | <Variable Declaration> “,” <Identifier>

<Statements> ::=  ε | <Statements> <Statement>

<Statement> ::= <AssignStatement> | <PrintStatement> | <CallStatement> | <ReturnStatement> | <IfStatement> | <WhileStatement> | <BreakStatement > | <ContinueStatement>
            
<AssignStatement> ::= <Identifier> "=" <Expression> ";"

<PrintStatement> ::= "print" "(" <StringConstant> <PActuals>")" ";"

<PActuals> ::= ε | <PActuals> “,” <Expression>

<CallStatement> ::= <CallExpression> ";"

<CallExpression> ::= <Identifier> "(" <Actuals> ")"

<Actuals> ::= ε | <Expression> <PActuals>

<ReturnStatement> ::= "return" ";" | "return" <Expression> ";"

<IfStatement> ::= “if” <TestExpr> <StatementsBlock> | “if” <TestExpr> <StatementsBlock> “else” <StatementsBlock>       

<TestExpr> ::= “(” <Expression> “)”

<StatementsBlock> ::= “{” <Statements> “}”

<WhileStatement> := “while” <TestExpr> <StatementsBlock>

<BreakStatement > := “break” “;”

<ContinueStatement> := “continue” “;”

<ReadInt> ::= "readint" "(" <StringConstant> ")" ";"

(注：运算符之间存在优先级，在用bison生成语法分析器时有定义)
<Expression> ::= <Expression> “+” <Expression> 
| <Expression> “-” <Expression> 
| <Expression> “*” <Expression> 
| <Expression> “/” <Expression> 
| <Expression> “%” <Expression> 
| <Expression> “>” <Expression>
| <Expression> “<” <Expression>
| <Expression> “>=” <Expression>
| <Expression> “<=” <Expression>
| <Expression> “==” <Expression>
| <Expression> “!=” <Expression>
| <Expression> “||” <Expression>
| <Expression> “&&” <Expression>
| “-” <Expression>
| “!” <Expression>
| <CallExpression>
| “(” <Expression> “)”
| <Identifier>
| <IntConstant>

<Identifier> ::= <Letter> 
            | "_"
| <Identifier> <Letter> 
| <Identifier> <Number> 
| <Identifier> "_"

<IntConstant> ::= <Number> | <Number> <IntConstant>

<StringConstant> ::= " \" [^\"\n]*\" "

<Number> ::= "0" | "1" | ... | "9"

<Letter> ::= "a" | "b" | ... | "z" | "A" | "B" | ... | "Z"
```
