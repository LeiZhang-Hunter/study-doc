下面我们从zend虚拟机的角度去看一段代码：

```
<?php
$a = "new string";


function test1()
{
        echo 3333;
}

function test2()
{
        echo 4444;
}

function test3()
{
        test2();
}

class Word{
        public function world()
        {
                $a = 1;
                test2();
                return $a;
        }
}

test1();

test3();

$w = new Word();
$i = $w->world();

```

我们使用gdb去调试

```
gdb php
break zend_execute
r test.php
```

我们继续向下看

```
(gdb) r test.php 
Starting program: /usr/local/bin/php test.php
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, zend_execute (op_array=op_array@entry=0x7fffeea802a0, return_value=return_value@entry=0x0)
    at /home/zhanglei/zhanglei/php-7.3.4/Zend/zend_vm_execute.h:60865
60865	{
(gdb) 

```

我们首先了解下op_array相关的全部数据结构

```
struct _zend_op_array {
	/* Common elements */
	zend_uchar type;
	zend_uchar arg_flags[3]; /* bitset of arg_info.pass_by_reference */
	uint32_t fn_flags;
	zend_string *function_name;
	zend_class_entry *scope;
	zend_function *prototype;
	uint32_t num_args;
	uint32_t required_num_args;
	zend_arg_info *arg_info;
	/* END of common elements */

	int cache_size;     /* number of run_time_cache_slots * sizeof(void*) */
	int last_var;       /* number of CV variables */
	uint32_t T;         /* number of temporary variables */
	uint32_t last;      /* number of opcodes */

	zend_op *opcodes;
	void **run_time_cache;
	HashTable *static_variables;
	zend_string **vars; /* names of CV variables */

	uint32_t *refcount;

	int last_live_range;
	int last_try_catch;
	zend_live_range *live_range;
	zend_try_catch_element *try_catch_array;

	zend_string *filename;
	uint32_t line_start;
	uint32_t line_end;
	zend_string *doc_comment;

	int last_literal;
	zval *literals;

	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
};


struct _zend_op {
	const void *handler;
	znode_op op1;
	znode_op op2;
	znode_op result;
	uint32_t extended_value;
	uint32_t lineno;
	zend_uchar opcode;
	zend_uchar op1_type;
	zend_uchar op2_type;
	zend_uchar result_type;
};


typedef union _znode_op {
	uint32_t      constant;
	uint32_t      var;
	uint32_t      num;
	uint32_t      opline_num; /*  Needs to be signed */
#if ZEND_USE_ABS_JMP_ADDR
	zend_op       *jmp_addr;
#else
	uint32_t      jmp_offset;
#endif
#if ZEND_USE_ABS_CONST_ADDR
	zval          *zv;
#endif
} znode_op;
```

我们使用gdb查看op_array

```
(gdb) p *op_array
$4 = {type = 2 '\002', arg_flags = "\000\000", fn_flags = 134217728, function_name = 0x0, scope = 0x0, prototype = 0x0, num_args = 0, 
  required_num_args = 0, arg_info = 0x0, cache_size = 40, last_var = 3, T = 9, last = 16, opcodes = 0x7fffeea70000, run_time_cache = 0x0, 
  static_variables = 0x0, vars = 0x7fffeea02090, refcount = 0x7fffeea8a000, last_live_range = 0, last_try_catch = 0, live_range = 0x0, 
  try_catch_array = 0x0, filename = 0x7fffeea6c360, line_start = 1, line_end = 35, doc_comment = 0x0, last_literal = 8, 
  literals = 0x7fffeea70200, reserved = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}}

```

我们现在来思考一下php的变量名字存放在哪

op_array的vars存放着所有变量的字面量

```
(gdb) p *op_array.vars[0].val@10
$19 = "a\000\000\000\000\000\000\000\000"
(gdb) p *op_array.vars[1].val@10
$20 = "w\000\000\000\000\000\000\000\000"
(gdb) p *op_array.vars[2].val@10
$21 = "i\000\000\000\000\000\000\000\000"
```

a w i 依次是我们代码中的  $a $w $i

他们的值存放在op_array的literals里

我们分别看下下面的值

```
(gdb) p op_array.literals.value.str.val@10
$27 = {"n", "e", "w", " ", "s", "t", "r", "i", "n", "g"}

(gdb) p op_array.literals[1].value.str.val@10
$42 = {"t", "e", "s", "t", "1", "", "", "", "\001", ""}
(gdb) p op_array.literals[2].value.str.val@10
$43 = {"t", "e", "s", "t", "3", "", "", "", "\001", ""}
(gdb) p op_array.literals[3].value.str.val@10
$44 = {"W", "o", "r", "d", "", "", "", "", "@", "\267"}
(gdb) p op_array.literals[4].value.str.val@10
$45 = {"w", "o", "r", "d", "", "", "", "", "\001", ""}
(gdb) p op_array.literals[5].value.str.val@10
$46 = {"w", "o", "r", "l", "d", "", "", "", "`", "\267"}
(gdb) p op_array.literals[6].value.str.val@10
$47 = {"w", "o", "r", "l", "d", "", "", "", "`", "\267"}

(gdb) p op_array.literals[7].u1.v.type
$50 = 4 '\004'
(gdb) p op_array.literals[7].value
$51 = {lval = 1, dval = 4.9406564584124654e-324, counted = 0x1, str = 0x1, arr = 0x1, obj = 0x1, res = 0x1, ref = 0x1, ast = 0x1, zv = 0x1, 
  ptr = 0x1, ce = 0x1, func = 0x1, ww = {w1 = 1, w2 = 0}}
(gdb) p op_array.literals[7].value.lval
$52 = 1


```

里面不仅存着new string，还存着类名和函数名字，而且还存放着$i的值