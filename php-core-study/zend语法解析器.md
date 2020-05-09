zend 语法解析使用的是bison语法解析器

词法分析（Lexical analysis或Scanning）和词法分析程序（Lexical analyzer或Scanner） 
　　词法分析阶段是编译过程的第一个阶段。这个阶段的任务是从左到右一个字符一个字符地读入源程序，即对构成源程序的字符流进行扫描然后根据构词规则识别单词(也称单词符号或符号)。词法分析程序实现这个任务。词法分析程序可以使用lex等工具自动生成
　　
语法分析是

语法分析（Syntax analysis或Parsing）和语法分析程序（Parser） 
　　语法分析是编译过程的一个逻辑阶段。语法分析的任务是在词法分析的基础上将单词序列组合成各类语法短语，如“程序”，“语句”，“表达式”等等.语法分析程序判断源程序在结构上是否正确.源程序的结构由上下文无关文法描述.
 
 zend用的是bison解析器
 
 我们写一个demo
 
 tipi中的波兰记号计算器
 
	 %{
	#define YYSTYPE double
	#include <stdio.h>
	#include <math.h>
	#include <ctype.h>
	int yylex (void);
	void yyerror (char const *);
	%}
	 
	%token NUM
	 
	%%
	input:    /* empty */
	     | input line
	    ;
	 
	line:     '\n'
	    | exp '\n'      { printf ("\t%.10g\n", $1); }
	;
	 
	exp:      NUM           { $$ = $1;           }
	   | exp exp '+'   { $$ = $1 + $2;      }
	    | exp exp '-'   { $$ = $1 - $2;      }
	    | exp exp '*'   { $$ = $1 * $2;      }
	    | exp exp '/'   { $$ = $1 / $2;      }
	     /* Exponentiation */
	    | exp exp '^'   { $$ = pow($1, $2); }
	    /* Unary minus    */
	    | exp 'n'       { $$ = -$1;          }
	;
	%%
	 
	#include <ctype.h>
	 
	int yylex (void) {
	       int c;
	 
	/* Skip white space.  */
	       while ((c = getchar ()) == ' ' || c == '\t') ;
	 
	/* Process numbers.  */
	       if (c == '.' || isdigit (c)) {
	       ungetc (c, stdin);
	       scanf ("%lf", &yylval);
	       return NUM;
	     }
	 
	       /* Return end-of-input.  */
	       if (c == EOF) return 0;
	 
	       /* Return a single char.  */
	       return c;
	}
	 
	void yyerror (char const *s) {
	    fprintf (stderr, "%s\n", s); 
	}
	 
	int main (void) {
	    return yyparse ();
	}
 
 我们使用bison生成c程序

``` 
 bison demoBison.l -o demoBison.c
 gcc demoBison.c -o main -lm
 ```
 
 我们运行程序
 

	 zhanglei@zhanglei-OptiPlex-9020:~/ourc/test$ ./main 
	4 0 +
		4
	4 1 -
		3
	1 3 -
		-2
	1 - 3

其实我发现一个问题就是你不管用re2C也好bison也罢他们最后都会生成一个c语言的库，这对于我们植入自己的c程序中就非常简单了，我们直接定义好include文件然后直接引入头文件和实现就可以将解析器嵌入到我们自己的c语言程序了

tipi又引出一个值得关注的点，就是zend如何把bison和re2c结合起来的

在这里简单说一下我对这些语法规则的简单理解

	```
	%token NUM
	```

这定义了一个符号名称

	```
	%%

	%%
	```
这之间定义了解析规则

编写一个函数，通过调用 yyparse() 来开始解析。详细的语法规则可以看这个网址

	https://www.ibm.com/developerworks/cn/linux/sdk/lex/
	
Lex 函数

	yylex()	这一函数开始分析。 它由 Lex 自动生成。
	$$：指向左部符号－－也就是冒号左边的符号－－的值。
	$1：指向右边第一个符号的值。
	$2：指向右边第二个符号的值。
	$3：指向右边第三个符号的值。
	……………依此类推。。。
		
	
简单了解一下bison和re2c我们就要进入重点了，就是php的

