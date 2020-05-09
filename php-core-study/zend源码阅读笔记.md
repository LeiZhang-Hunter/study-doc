年底了空闲一些，开始看zend虚拟机，还有几天过年了，写下这篇学习笔记，简单的介绍一下我近期对zend虚拟机的学习

我最近学习了zend虚拟机，首先了解到了一个东西

re2c+bison

php正是通过这个东西对php脚本进行的解析，这个我的初步了解是在php 胖子的tipi上

http://www.php-internals.com/book/?p=chapt07/07-00-zend-vm

然后我对里面的demo语法进行了运行了解到 ,不管是re2c也好或者是bison也罢，我们都可以生成对应的c语言解析文件，只要我们封装好良好的头文件，再引入对应的实现文件，我们就可以实现这个解析

我们使用

```
re2c -F -c -o zend_language_scanner2.c zend_language_scanner.l

```

我们就可以实现自己的c源码文件库

bison语法解析也是这个样子的 ，用一个最简单的变量复制为例子，在zend底层中的识别规则是，这样我们使用re2c 可以分析出对应的token然后使用语法分析器将对应的token挂到对应的ast上,这个re2c和bison我也不是很熟，只是运行了一些简单的demo，直接贴出几个php片段吧

词法分析:

```
<ST_DOUBLE_QUOTES,ST_HEREDOC,ST_BACKQUOTE>"$"{LABEL}"->"[a-zA-Z_\x80-\xff] {
	yyless(yyleng - 3);
	yy_push_state(ST_LOOKING_FOR_PROPERTY);
	RETURN_TOKEN_WITH_STR(T_VARIABLE, 1);
}

```

语法解析:
```
  case 386:

    { (yyval.ast) = zend_ast_list_add((yyvsp[-2].ast), (yyvsp[0].ast)); }

    break;

  case 387:

    { (yyval.ast) = zend_ast_create_list(1, ZEND_AST_CLOSURE_USES, (yyvsp[0].ast)); }

    break;
```



词法分析是将一句花给拆成一个一个的单词

语法解析入口是yylex，zend把他给改名为了

zendparse 

使用的入口是在

```
static zend_op_array *zend_compile(int type)
{
	zend_op_array *op_array = NULL;
	zend_bool original_in_compilation = CG(in_compilation);

	CG(in_compilation) = 1;
	CG(ast) = NULL;
	CG(ast_arena) = zend_arena_create(1024 * 32);

	if (!zendparse()) {
		int last_lineno = CG(zend_lineno);
		zend_file_context original_file_context;
		zend_oparray_context original_oparray_context;
		zend_op_array *original_active_op_array = CG(active_op_array);

		op_array = emalloc(sizeof(zend_op_array));
		init_op_array(op_array, type, INITIAL_OP_ARRAY_SIZE);
		CG(active_op_array) = op_array;

		if (zend_ast_process) {
			zend_ast_process(CG(ast));
		}

		zend_file_context_begin(&original_file_context);
		zend_oparray_context_begin(&original_oparray_context);
		zend_compile_top_stmt(CG(ast));
		CG(zend_lineno) = last_lineno;
		zend_emit_final_return(type == ZEND_USER_FUNCTION);
		op_array->line_start = 1;
		op_array->line_end = last_lineno;
		pass_two(op_array);
		zend_oparray_context_end(&original_oparray_context);
		zend_file_context_end(&original_file_context);

		CG(active_op_array) = original_active_op_array;
	}

	zend_ast_destroy(CG(ast));
	zend_arena_destroy(CG(ast_arena));

	CG(in_compilation) = original_in_compilation;

	return op_array;
}
```
当调用到这里zend就展开了语法解析和词法分析

zendparse之后，zend将php语法挂到了ast树上

```
struct _zend_compiler_globals {
	zend_stack loop_var_stack;

	zend_class_entry *active_class_entry;

	zend_string *compiled_filename;

	int zend_lineno;

	zend_op_array *active_op_array;

	HashTable *function_table;	/* function symbol table */
	HashTable *class_table;		/* class table */

	HashTable filenames_table;

	HashTable *auto_globals;

	zend_bool parse_error;
	zend_bool in_compilation;
	zend_bool short_tags;

	zend_bool unclean_shutdown;

	zend_bool ini_parser_unbuffered_errors;

	zend_llist open_files;

	struct _zend_ini_parser_param *ini_parser_param;

	uint32_t start_lineno;
	zend_bool increment_lineno;

	zend_string *doc_comment;
	uint32_t extra_fn_flags;

	uint32_t compiler_options; /* set of ZEND_COMPILE_* constants */

	zend_oparray_context context;
	zend_file_context file_context;

	zend_arena *arena;

	HashTable interned_strings;

	const zend_encoding **script_encoding_list;
	size_t script_encoding_list_size;
	zend_bool multibyte;
	zend_bool detect_unicode;
	zend_bool encoding_declared;

	zend_ast *ast;
	zend_arena *ast_arena;

	zend_stack delayed_oplines_stack;

#ifdef ZTS
	zval **static_members_table;
	int last_static_member;
#endif
};
```

我们在这里简单看几个ast的函数

zend_ast_alloc？他做了什么呢?

```
static inline void *zend_ast_alloc(size_t size) {
	return zend_arena_alloc(&CG(ast_arena), size);
}
```

我认为这是一个环形内存池,代码展示

```
static zend_always_inline void* zend_arena_alloc(zend_arena **arena_ptr, size_t size)
{
	zend_arena *arena = *arena_ptr;
	char *ptr = arena->ptr;

	size = ZEND_MM_ALIGNED_SIZE(size);

	if (EXPECTED(size <= (size_t)(arena->end - ptr))) {
		arena->ptr = ptr + size;
	} else {
		size_t arena_size =
			UNEXPECTED((size + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena))) > (size_t)(arena->end - (char*) arena)) ?
				(size + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena))) :
				(size_t)(arena->end - (char*) arena);
		zend_arena *new_arena = (zend_arena*)emalloc(arena_size);

		ptr = (char*) new_arena + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena));
		new_arena->ptr = (char*) new_arena + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena)) + size;
		new_arena->end = (char*) new_arena + arena_size;
		new_arena->prev = arena;
		*arena_ptr = new_arena;
	}

	return (void*) ptr;
}
```

如果说剩余的长度充足，那么是从头开始偏移size,返回对应的尺寸


```
arena->ptr = ptr + size;
```

如果说环形内存池有充足的尺寸
```
		size_t arena_size =
			UNEXPECTED((size + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena))) > (size_t)(arena->end - (char*) arena)) ?
				(size + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena))) :
				(size_t)(arena->end - (char*) arena);
		zend_arena *new_arena = (zend_arena*)emalloc(arena_size);

		ptr = (char*) new_arena + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena));
		new_arena->ptr = (char*) new_arena + ZEND_MM_ALIGNED_SIZE(sizeof(zend_arena)) + size;
		new_arena->end = (char*) new_arena + arena_size;
		new_arena->prev = arena;
		*arena_ptr = new_arena;
```

在说一个值得注意的内容，用一个例子来说明

```
static zend_always_inline zend_ast * zend_ast_create_zval_int(zval *zv, uint32_t attr, uint32_t lineno) {
	zend_ast_zval *ast;

	ast = zend_ast_alloc(sizeof(zend_ast_zval));
	ast->kind = ZEND_AST_ZVAL;
	ast->attr = attr;
	ZVAL_COPY_VALUE(&ast->val, zv);
	Z_LINENO(ast->val) = lineno;
	return (zend_ast *) ast;
}
```

注意一个内容 如果是一个变量:

那么kind是ZEND_AST_ZVAL，存储结构变为了zend_ast_zval

则在申请一块内存，并且把前向指针记录为之前的areana

分析完了内存的申请 还有一个重要的步骤我们需要再看一下树节点的插入

```
ZEND_API zend_ast * ZEND_FASTCALL zend_ast_list_add(zend_ast *ast, zend_ast *op) {
	zend_ast_list *list = zend_ast_get_list(ast);
	if (list->children >= 4 && is_power_of_two(list->children)) {
			list = zend_ast_realloc(list,
			zend_ast_list_size(list->children), zend_ast_list_size(list->children * 2));
	}
	list->child[list->children++] = op;
	return (zend_ast *) list;
}

```

添加列表直接挂载到child上

树的构造有前序遍历  后序遍历 和 中序遍历,在这里是中序遍历，参考的文章是:

https://segmentfault.com/a/1190000019097615

我们还要认识几个ast的主要结构:

```
/* Same as zend_ast, but with children count, which is updated dynamically */
typedef struct _zend_ast_list {
	zend_ast_kind kind;
	zend_ast_attr attr;
	uint32_t lineno;
	uint32_t children;
	zend_ast *child[1];
} zend_ast_list;

/* Lineno is stored in val.u2.lineno */
typedef struct _zend_ast_zval {
	zend_ast_kind kind;
	zend_ast_attr attr;
	zval val;
} zend_ast_zval;

/* Separate structure for function and class declaration, as they need extra information. */
typedef struct _zend_ast_decl {
	zend_ast_kind kind;
	zend_ast_attr attr; /* Unused - for structure compatibility */
	uint32_t start_lineno;
	uint32_t end_lineno;
	uint32_t flags;
	unsigned char *lex_pos;
	zend_string *doc_comment;
	zend_string *name;
	zend_ast *child[4];
} zend_ast_decl;
```

这几个结构是十分重要的结构比如说ZEND_AST_CALL对应的结构体是_zend_ast_decl,ZEND_AST_ZVAL是_zend_ast_zval.....

后来我自己写了一个例子，自己调试一把

测试文件：

```
<?php

$a = 1;
$b = 2;
$c = 3;

$b = $a + $b + $c;

var_dump($b);
var_dump($b);
var_dump($b);
var_dump($b);
var_dump($b);
var_dump($b);
var_dump($b);
var_dump($b);
var_dump($b);
var_dump($b);
var_dump($b);
```

开始用gdb来进行调试

	gdb php
	
	break zend_compile_top_stmt
	
	run test.php
	
然后程序定到了

```
(gdb) run test.php 
Starting program: /usr/bin/php test.php
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, zend_compile_top_stmt (ast=0x7fffeea83530) at /home/zhanglei/ourc/php-7.3.11/Zend/zend_compile.c:8190
8190		if (!ast) {

```

我们从这个语法树上去分析

```
(gdb) p *compiler_globals.ast
$2 = {kind = 132, attr = 0, lineno = 1, child = {0xf}}

```

我们看到zend的kind是132,132是ZEND_AST_STMT_LIST
```
(gdb) p *compiler_globals.ast.child@15
$17 = {0xf, 0x7fffeea83088, 0x7fffeea830e0, 0x7fffeea83138, 0x7fffeea83220, 0x7fffeea832a8, 0x7fffeea83380, 0x7fffeea83408, 0x7fffeea83490, 0x7fffeea83518, 0x7fffeea83630, 0x7fffeea836b8, 
  0x7fffeea83740, 0x7fffeea837c8, 0x7fffeea83850}
(gdb) 
```

下面我会一个一个分析这个list下面的每一个ast类型

```
(gdb) p *compiler_globals.ast.child[1]
$18 = {kind = 517, attr = 0, lineno = 3, child = {0x7fffeea83060}}
(gdb) p *compiler_globals.ast.child[2]
$19 = {kind = 517, attr = 0, lineno = 4, child = {0x7fffeea830b8}}
(gdb) p *compiler_globals.ast.child[3]
$20 = {kind = 517, attr = 0, lineno = 5, child = {0x7fffeea83110}}
(gdb) p *compiler_globals.ast.child[4]
$21 = {kind = 517, attr = 0, lineno = 7, child = {0x7fffeea83168}}
(gdb) p *compiler_globals.ast.child[5]

```
517是ZEND_AST_ASSIGN 赋值,是4个，我们在程序中正是使用了4个赋值

```
$a = 1;
$b = 2;
$c = 3;

$b = $a + $b + $c;
```


后面的几个

```
(gdb) p *compiler_globals.ast.child[5]
$22 = {kind = 515, attr = 0, lineno = 9, child = {0x7fffeea83238}}
(gdb) p *compiler_globals.ast.child[6]
$23 = {kind = 515, attr = 0, lineno = 10, child = {0x7fffeea83310}}
(gdb) p *compiler_globals.ast.child[7]
$24 = {kind = 515, attr = 0, lineno = 11, child = {0x7fffeea83398}}
(gdb) p *compiler_globals.ast.child[8]
$25 = {kind = 515, attr = 0, lineno = 12, child = {0x7fffeea83420}}
(gdb) p *compiler_globals.ast.child[9]
$26 = {kind = 515, attr = 0, lineno = 13, child = {0x7fffeea834a8}}
(gdb) p *compiler_globals.ast.child[10]
$27 = {kind = 515, attr = 0, lineno = 14, child = {0x7fffeea835c0}}
(gdb) p *compiler_globals.ast.child[11]
$28 = {kind = 515, attr = 0, lineno = 15, child = {0x7fffeea83648}}
(gdb) p *compiler_globals.ast.child[12]
$29 = {kind = 515, attr = 0, lineno = 16, child = {0x7fffeea836d0}}
(gdb) p *compiler_globals.ast.child[13]
$30 = {kind = 515, attr = 0, lineno = 17, child = {0x7fffeea83758}}
(gdb) p *compiler_globals.ast.child[14]
$31 = {kind = 515, attr = 0, lineno = 18, child = {0x7fffeea837e0}}
(gdb) p *compiler_globals.ast.child[15]
$32 = {kind = 515, attr = 0, lineno = 19, child = {0x7fffeea83868}}
```

都是515，在代码中是ZEND_AST_CALL代表的是函数调用 现在我们再继续向下看

其实看到这里我开始思考了一个问题就是以$a = 1,为例子(ZEND_AST_ASSIGN)

$符很可能被分析器过滤掉 ，那么剩下的就是$a 的"a"存在了哪里，1放到了哪里

我发现赋值的话直接把值存储到ast child转化的zend_ast_zval*的zval里

```
(gdb) p (*(*(zend_ast_zval*)(compiler_globals.ast.child[1])).val.value.str.val)
$44 = 1 '\001'
(gdb) p (*(*(zend_ast_zval*)(compiler_globals.ast.child[2])).val.value.str.val)
$45 = 2 '\002'
(gdb) p (*(*(zend_ast_zval*)(compiler_globals.ast.child[3])).val.value.str.val)
$46 = 3 '\003'
```

分别存储了1,2,3但还是思考a的位置在哪里,于是我全局搜索了ZEND_AST_VAR，发现了这么一段代码

```
static int zend_try_compile_cv(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast *name_ast = ast->child[0];
	if (name_ast->kind == ZEND_AST_ZVAL) {
		zval *zv = zend_ast_get_zval(name_ast);
		zend_string *name;

		if (EXPECTED(Z_TYPE_P(zv) == IS_STRING)) {
			name = zval_make_interned_string(zv);
		} else {
			name = zend_new_interned_string(zval_get_string_func(zv));
		}

		if (zend_is_auto_global(name)) {
			return FAILURE;
		}

		result->op_type = IS_CV;
		result->u.op.var = lookup_cv(CG(active_op_array), name);

		if (UNEXPECTED(Z_TYPE_P(zv) != IS_STRING)) {
			zend_string_release_ex(name, 0);
		}

		return SUCCESS;
	}

	return FAILURE;
}
```

说明了他是一个字符串！！因为这一段代码
```
zend_string *name;

if (EXPECTED(Z_TYPE_P(zv) == IS_STRING)) {
	name = zval_make_interned_string(zv);
} else {
	name = zend_new_interned_string(zval_get_string_func(zv));
}
```

好了知道了变量名字的位置我们使用gdb调试吧

```
(gdb) p *(*(zend_ast_zval*)*compiler_globals.ast.child[2].child.child).val.value.str
$96 = {gc = {refcount = 1, u = {type_info = 454}}, h = 9223372036854953479, len = 1, val = "b"}
(gdb) p *(*(zend_ast_zval*)*compiler_globals.ast.child[1].child.child).val.value.str
$97 = {gc = {refcount = 1, u = {type_info = 454}}, h = 9223372036854953478, len = 1, val = "a"}
(gdb) p *(*(zend_ast_zval*)*compiler_globals.ast.child[3].child.child).val.value.str
$98 = {gc = {refcount = 1, u = {type_info = 454}}, h = 9223372036854953480, len = 1, val = "c"}

```

到了这里我们看到了abc的位置

那我们再思考这段代码中赋值是如何存储的？然后我们看右子叶中存储的是type 64 ZEND_AST_ZVAL

其实这里就说明问题了赋值的数被存放到这里，再用gdb看

```
(gdb) p *(zend_ast_zval*)compiler_globals.ast.child[3].child[1]
$158 = {kind = 64, attr = 0, val = {value = {lval = 3, dval = 1.4821969375237396e-323, counted = 0x3, str = 0x3, arr = 0x3, obj = 0x3, res = 0x3, ref = 0x3, ast = 0x3, zv = 0x3, ptr = 0x3, ce = 0x3, 
      func = 0x3, ww = {w1 = 3, w2 = 0}}, u1 = {v = {type = 4 '\004', type_flags = 0 '\000', u = {call_info = 0, extra = 0}}, type_info = 4}, u2 = {next = 5, cache_slot = 5, opline_num = 5, lineno = 5, 
      num_args = 5, fe_pos = 5, fe_iter_idx = 5, access_flags = 5, property_guard = 5, constant_flags = 5, extra = 5}}}

```

透过这里我们看到了zend的u1.v.type是4对应的宏定义是IS_LONG

这里就是我们存储的值

说实话读代码读到这里我就开始思考一个问题为什么zend_ast 和 zend_ast_zval的kind会一样呢？思考了一会儿我响起了多年前学习的滴水逆向教程的汇编课程，c语言开头kind都是一样的，我们看结构体

```
struct _zend_ast {
	zend_ast_kind kind; /* Type of the node (ZEND_AST_* enum constant) */
	zend_ast_attr attr; /* Additional attribute, use depending on node type */
	uint32_t lineno;    /* Line number */
	zend_ast *child[1]; /* Array of children (using struct hack) */
};

/* Same as zend_ast, but with children count, which is updated dynamically */
typedef struct _zend_ast_list {
	zend_ast_kind kind;
	zend_ast_attr attr;
	uint32_t lineno;
	uint32_t children;
	zend_ast *child[1];
} zend_ast_list;
```

你会发现内存分配的位置kind都是在第一个成员变量的位置，结果很明显了，赋值的话内存前几个位置是相同的,后面是柔性数组存放zval的，不得不说这个数据结构设计的不错
```
zend_ast_kind kind;
zend_ast_attr attr;
uint32_t lineno;
```

所以值也会对齐

我们在继续看一下$b = $a + $b + $c;zend是如何处理的

用gdb看第4个child

```
(gdb) p *compiler_globals.ast.child[4]
$161 = {kind = 517, attr = 0, lineno = 7, child = {0x7fffeea83168}}

```

这里的kind是517还是赋值

然后查看child内容

```
(gdb) p *(*(zend_ast_zval*)*compiler_globals.ast.child[4].child.child).val.value.str
$177 = {gc = {refcount = 1, u = {type_info = 454}}, h = 9223372036854953479, len = 1, val = "b"}

```

第一个位置存放的是声明

gdb打印第二个位置:

```
(gdb) p (*compiler_globals.ast.child[4].child[1])
$182 = {kind = 520, attr = 1, lineno = 7, child = {0x7fffeea831c8}}
```

这个520对应的kind是ZEND_AST_BINARY_OP，我们在php源码中去找对应的代码看看使用情况

```
		case ZEND_AST_BINARY_OP:
			switch (ast->attr) {
				case ZEND_ADD:                 BINARY_OP(" + ",   200, 200, 201);
				case ZEND_SUB:                 BINARY_OP(" - ",   200, 200, 201);
				case ZEND_MUL:                 BINARY_OP(" * ",   210, 210, 211);
				case ZEND_DIV:                 BINARY_OP(" / ",   210, 210, 211);
				case ZEND_MOD:                 BINARY_OP(" % ",   210, 210, 211);
				case ZEND_SL:                  BINARY_OP(" << ",  190, 190, 191);
				case ZEND_SR:                  BINARY_OP(" >> ",  190, 190, 191);
				case ZEND_CONCAT:              BINARY_OP(" . ",   200, 200, 201);
				case ZEND_BW_OR:               BINARY_OP(" | ",   140, 140, 141);
				case ZEND_BW_AND:              BINARY_OP(" & ",   160, 160, 161);
				case ZEND_BW_XOR:              BINARY_OP(" ^ ",   150, 150, 151);
				case ZEND_IS_IDENTICAL:        BINARY_OP(" === ", 170, 171, 171);
				case ZEND_IS_NOT_IDENTICAL:    BINARY_OP(" !== ", 170, 171, 171);
				case ZEND_IS_EQUAL:            BINARY_OP(" == ",  170, 171, 171);
				case ZEND_IS_NOT_EQUAL:        BINARY_OP(" != ",  170, 171, 171);
				case ZEND_IS_SMALLER:          BINARY_OP(" < ",   180, 181, 181);
				case ZEND_IS_SMALLER_OR_EQUAL: BINARY_OP(" <= ",  180, 181, 181);
				case ZEND_POW:                 BINARY_OP(" ** ",  250, 251, 250);
				case ZEND_BOOL_XOR:            BINARY_OP(" xor ",  40,  40,  41);
				case ZEND_SPACESHIP:           BINARY_OP(" <=> ", 180, 181, 181);
				EMPTY_SWITCH_DEFAULT_CASE();
```


我们再继续看var_dump

```
(gdb) p (*(*(zend_ast_zval*)(compiler_globals.ast.child[6].child[0])).val.value.str.val@10)
$27 = "var_dump\000"
```

第一个位置放的是函数名字，然后我们在看第二个位置


```
(gdb) p *((compiler_globals.ast.child[5].child[1]))
$295 = {kind = 128, attr = 0, lineno = 9, child = {0x1}}
```

kind是128对应的是ZEND_AST_ARG_LIST 这是一个参数列表，我们看代码

然后我再代码中找到了这么一段

```
zend_ast_list *list = zend_ast_get_list(args->child[1]->child[1]);
```

也就是说我们要看child的1的位置而不是0的位置，好了到这里我们再继续用gdb做调试，这样我们就看到了我们的传入参数

```
(gdb) p *(*(zend_ast_zval*)*((compiler_globals.ast.child[5].child[1].child[1].child))).val.value.str
$305 = {gc = {refcount = 1, u = {type_info = 454}}, h = 9223372036854953479, len = 1, val = "b"}
```


