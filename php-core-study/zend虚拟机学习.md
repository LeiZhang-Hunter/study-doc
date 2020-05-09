#Zend虚拟机部分的学习

之前大体看了zend ast语法部分的解析，也用gdb大体看了zend语法树的运行结果，但是读完之后我会思考问题，就是语法全部挂到ast上，那么ast是如何变成一个个opcode然后组成op_array的呢，zend虚拟机又是怎么一个个执行这个opcode的呢？

记得2年前，我面试有一些很资深的程序员就会问我，你了解op_array呢？但是当年没看过php源码，但是写过一些扩展尤其是include函数，但是就是不能理解op_array，后来我问别人op_array是什么呢？别人告诉我op_array是php中的一个个函数，保存着一个个php的函数栈，这阵子看起来确实是这个样子。

我先来思考和探究第一个脑海中的问题，就是zend ast语法部分的解析是如何转化为op_code的呢？

我们继续看之前看的一个函数

```
void zend_compile_stmt(zend_ast *ast) /* {{{ */
{
	if (!ast) {
		return;
	}

	CG(zend_lineno) = ast->lineno;

	if ((CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO) && !zend_is_unticked_stmt(ast)) {
		zend_do_extended_info();
	}

	switch (ast->kind) {
		case ZEND_AST_STMT_LIST:
			zend_compile_stmt_list(ast);
			break;
		case ZEND_AST_GLOBAL:
			zend_compile_global_var(ast);
			break;
		case ZEND_AST_STATIC:
			zend_compile_static_var(ast);
			break;
		case ZEND_AST_UNSET:
			zend_compile_unset(ast);
			break;
		case ZEND_AST_RETURN:
			zend_compile_return(ast);
			break;
		case ZEND_AST_ECHO:
			zend_compile_echo(ast);
			break;
		case ZEND_AST_THROW:
			zend_compile_throw(ast);
			break;
		case ZEND_AST_BREAK:
		case ZEND_AST_CONTINUE:
			zend_compile_break_continue(ast);
			break;
		case ZEND_AST_GOTO:
			zend_compile_goto(ast);
			break;
		case ZEND_AST_LABEL:
			zend_compile_label(ast);
			break;
		case ZEND_AST_WHILE:
			zend_compile_while(ast);
			break;
		case ZEND_AST_DO_WHILE:
			zend_compile_do_while(ast);
			break;
		case ZEND_AST_FOR:
			zend_compile_for(ast);
			break;
		case ZEND_AST_FOREACH:
			zend_compile_foreach(ast);
			break;
		case ZEND_AST_IF:
			zend_compile_if(ast);
			break;
		case ZEND_AST_SWITCH:
			zend_compile_switch(ast);
			break;
		case ZEND_AST_TRY:
			zend_compile_try(ast);
			break;
		case ZEND_AST_DECLARE:
			zend_compile_declare(ast);
			break;
		case ZEND_AST_FUNC_DECL:
		case ZEND_AST_METHOD:
			zend_compile_func_decl(NULL, ast);
			break;
		case ZEND_AST_PROP_DECL:
			zend_compile_prop_decl(ast);
			break;
		case ZEND_AST_CLASS_CONST_DECL:
			zend_compile_class_const_decl(ast);
			break;
		case ZEND_AST_USE_TRAIT:
			zend_compile_use_trait(ast);
			break;
		case ZEND_AST_CLASS:
			zend_compile_class_decl(ast);
			break;
		case ZEND_AST_GROUP_USE:
			zend_compile_group_use(ast);
			break;
		case ZEND_AST_USE:
			zend_compile_use(ast);
			break;
		case ZEND_AST_CONST_DECL:
			zend_compile_const_decl(ast);
			break;
		case ZEND_AST_NAMESPACE:
			zend_compile_namespace(ast);
			break;
		case ZEND_AST_HALT_COMPILER:
			zend_compile_halt_compiler(ast);
			break;
		default:
		{
			znode result;
			zend_compile_expr(&result, ast);
			zend_do_free(&result);
		}
	}

	if (FC(declarables).ticks && !zend_is_unticked_stmt(ast)) {
		zend_emit_tick();
	}
}
```

继续看我们之前昨天例子中的赋值操作

```
void zend_compile_assign(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast *var_ast = ast->child[0];
	zend_ast *expr_ast = ast->child[1];

	znode var_node, expr_node;
	zend_op *opline;
	uint32_t offset;

	if (is_this_fetch(var_ast)) {
		zend_error_noreturn(E_COMPILE_ERROR, "Cannot re-assign $this");
	}

	zend_ensure_writable_variable(var_ast);

	switch (var_ast->kind) {
		case ZEND_AST_VAR:
		case ZEND_AST_STATIC_PROP:
			offset = zend_delayed_compile_begin();
			zend_delayed_compile_var(&var_node, var_ast, BP_VAR_W);
			zend_compile_expr(&expr_node, expr_ast);
			zend_delayed_compile_end(offset);
			zend_emit_op(result, ZEND_ASSIGN, &var_node, &expr_node);
			return;
		case ZEND_AST_DIM:
			offset = zend_delayed_compile_begin();
			zend_delayed_compile_dim(result, var_ast, BP_VAR_W);

			if (zend_is_assign_to_self(var_ast, expr_ast)
			 && !is_this_fetch(expr_ast)) {
				/* $a[0] = $a should evaluate the right $a first */
				znode cv_node;

				if (zend_try_compile_cv(&cv_node, expr_ast) == FAILURE) {
					zend_compile_simple_var_no_cv(&expr_node, expr_ast, BP_VAR_R, 0);
				} else {
					zend_emit_op(&expr_node, ZEND_QM_ASSIGN, &cv_node, NULL);
				}
			} else {
				zend_compile_expr(&expr_node, expr_ast);
			}

			opline = zend_delayed_compile_end(offset);
			opline->opcode = ZEND_ASSIGN_DIM;

			opline = zend_emit_op_data(&expr_node);
			return;
		case ZEND_AST_PROP:
			offset = zend_delayed_compile_begin();
			zend_delayed_compile_prop(result, var_ast, BP_VAR_W);
			zend_compile_expr(&expr_node, expr_ast);

			opline = zend_delayed_compile_end(offset);
			opline->opcode = ZEND_ASSIGN_OBJ;

			zend_emit_op_data(&expr_node);
			return;
		case ZEND_AST_ARRAY:
			if (zend_propagate_list_refs(var_ast)) {
				if (!zend_is_variable(expr_ast)) {
					zend_error_noreturn(E_COMPILE_ERROR,
						"Cannot assign reference to non referencable value");
				}

				zend_compile_var(&expr_node, expr_ast, BP_VAR_W);
				/* MAKE_REF is usually not necessary for CVs. However, if there are
				 * self-assignments, this forces the RHS to evaluate first. */
				if (expr_node.op_type != IS_CV
						|| zend_list_has_assign_to_self(var_ast, expr_ast)) {
					zend_emit_op(&expr_node, ZEND_MAKE_REF, &expr_node, NULL);
				}
			} else {
				if (zend_list_has_assign_to_self(var_ast, expr_ast)) {
					/* list($a, $b) = $a should evaluate the right $a first */
					znode cv_node;

					if (zend_try_compile_cv(&cv_node, expr_ast) == FAILURE) {
						zend_compile_simple_var_no_cv(&expr_node, expr_ast, BP_VAR_R, 0);
					} else {
						zend_emit_op(&expr_node, ZEND_QM_ASSIGN, &cv_node, NULL);
					}
				} else {
					zend_compile_expr(&expr_node, expr_ast);
				}
			}

			zend_compile_list_assign(result, var_ast, &expr_node, var_ast->attr);
			return;
		EMPTY_SWITCH_DEFAULT_CASE();
	}
}
```

我们看到之中调用了两个关键函数 zend_emit_op_data 和 zend_emit_op

```
static inline zend_op *zend_emit_op_data(znode *value) /* {{{ */
{
	return zend_emit_op(NULL, ZEND_OP_DATA, value, NULL);
}
```

我们看一下zend_emit_op里做了什么

```
static zend_op *zend_emit_op(znode *result, zend_uchar opcode, znode *op1, znode *op2) /* {{{ */
{
	zend_op *opline = get_next_op(CG(active_op_array));
	opline->opcode = opcode;

	if (op1 != NULL) {
		SET_NODE(opline->op1, op1);
	}

	if (op2 != NULL) {
		SET_NODE(opline->op2, op2);
	}

	zend_check_live_ranges(opline);

	if (result) {
		zend_make_var_result(result, opline);
	}
	return opline;
}
```
再看一下get_next_op做了什么？



```
static zend_op *get_next_op(zend_op_array *op_array)
{
	uint32_t next_op_num = op_array->last++;
	zend_op *next_op;

	if (UNEXPECTED(next_op_num >= CG(context).opcodes_size)) {
		CG(context).opcodes_size *= 4;
		op_array->opcodes = erealloc(op_array->opcodes, CG(context).opcodes_size * sizeof(zend_op));
	}

	next_op = &(op_array->opcodes[next_op_num]);

	init_op(next_op);

	return next_op;
}
```

CG(active_op_array)的最后一个op加1，看一下init_op

```
static void init_op(zend_op *op)
{
	MAKE_NOP(op);
	op->extended_value = 0;
	op->lineno = CG(zend_lineno);
}
```

再看MAKE_NOP

```
#define MAKE_NOP(opline) do { \
	(opline)->op1.num = 0; \
	(opline)->op2.num = 0; \
	(opline)->result.num = 0; \
	(opline)->opcode = ZEND_NOP; \
	(opline)->op1_type =  IS_UNUSED; \
	(opline)->op2_type = IS_UNUSED; \
	(opline)->result_type = IS_UNUSED; \
} while (0)
```

这个目的是为了初始化这个op,然后

说到这里我们先要看一下op_array的数据结构get_next_op将这个新的op返回，然后设置他的操作数和返回结果

我们看一下其中的关键数据结构
```

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
```

看到了这里我是有疑惑的，因为如果说一个函数是一个op_array,一个op_array之中是很多opcode,里面装载了我们php代码中的运行程序，那么这些opcode是如何确认我们要装载到哪一个op_array里呢？是不是每次有函数调用的时候就会切换我们的CG(active_op_array)呢？我们看一下一个函数调用的部分代码，观察zend_compile_func_decl

```
void zend_compile_func_decl(znode *result, zend_ast *ast) /* {{{ */
{
	zend_ast_decl *decl = (zend_ast_decl *) ast;
	zend_ast *params_ast = decl->child[0];
	zend_ast *uses_ast = decl->child[1];
	zend_ast *stmt_ast = decl->child[2];
	zend_ast *return_type_ast = decl->child[3];
	zend_bool is_method = decl->kind == ZEND_AST_METHOD;

	zend_op_array *orig_op_array = CG(active_op_array);
	zend_op_array *op_array = zend_arena_alloc(&CG(arena), sizeof(zend_op_array));
	zend_oparray_context orig_oparray_context;

	init_op_array(op_array, ZEND_USER_FUNCTION, INITIAL_OP_ARRAY_SIZE);

	op_array->fn_flags |= (orig_op_array->fn_flags & ZEND_ACC_STRICT_TYPES);
	op_array->fn_flags |= decl->flags;
	op_array->line_start = decl->start_lineno;
	op_array->line_end = decl->end_lineno;
	if (decl->doc_comment) {
		op_array->doc_comment = zend_string_copy(decl->doc_comment);
	}
	if (decl->kind == ZEND_AST_CLOSURE) {
		op_array->fn_flags |= ZEND_ACC_CLOSURE;
	}

	if (is_method) {
		zend_bool has_body = stmt_ast != NULL;
		zend_begin_method_decl(op_array, decl->name, has_body);
	} else {
		zend_begin_func_decl(result, op_array, decl);
		if (uses_ast) {
			zend_compile_closure_binding(result, op_array, uses_ast);
		}
	}

	CG(active_op_array) = op_array;

	zend_oparray_context_begin(&orig_oparray_context);

	if (CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO) {
		zend_op *opline_ext = zend_emit_op(NULL, ZEND_EXT_NOP, NULL, NULL);
		opline_ext->lineno = decl->start_lineno;
	}

	{
		/* Push a separator to the loop variable stack */
		zend_loop_var dummy_var;
		dummy_var.opcode = ZEND_RETURN;

		zend_stack_push(&CG(loop_var_stack), (void *) &dummy_var);
	}

	zend_compile_params(params_ast, return_type_ast);
	if (CG(active_op_array)->fn_flags & ZEND_ACC_GENERATOR) {
		zend_mark_function_as_generator();
		zend_emit_op(NULL, ZEND_GENERATOR_CREATE, NULL, NULL);
	}
	if (uses_ast) {
		zend_compile_closure_uses(uses_ast);
	}
	zend_compile_stmt(stmt_ast);

	if (is_method) {
		zend_check_magic_method_implementation(
			CG(active_class_entry), (zend_function *) op_array, E_COMPILE_ERROR);
	}

	/* put the implicit return on the really last line */
	CG(zend_lineno) = decl->end_lineno;

	zend_do_extended_info();
	zend_emit_final_return(0);

	pass_two(CG(active_op_array));
	zend_oparray_context_end(&orig_oparray_context);

	/* Pop the loop variable stack separator */
	zend_stack_del_top(&CG(loop_var_stack));

	CG(active_op_array) = orig_op_array;
}
```

zend_oparray_context_begin 是用来初始化CG(context)的，我们看到一个很关键的点，就是在

```
CG(active_op_array) = op_array;
zend_oparray_context_begin(&orig_oparray_context);

....
...

zend_compile_params(params_ast, return_type_ast);
if (CG(active_op_array)->fn_flags & ZEND_ACC_GENERATOR) {
	zend_mark_function_as_generator();
	zend_emit_op(NULL, ZEND_GENERATOR_CREATE, NULL, NULL);
}
if (uses_ast) {
	zend_compile_closure_uses(uses_ast);
}
zend_compile_stmt(stmt_ast);

...
pass_two(CG(active_op_array));
...
CG(active_op_array) = orig_op_array;
```

就是在切换CG(active_op_array) 之后，又一次调用了zend_compile_stmt开始生成对应op_array的op_code了，并且最后会把CG(active_op_array)复原

我们发现这个函数是不是和外面的这个zend_compile的流程非常的像，那我们再看一个关键函数pass_two，这个函数是做什么的呢，我们看pass_two

```
ZEND_API int pass_two(zend_op_array *op_array)
{
	zend_op *opline, *end;

	if (!ZEND_USER_CODE(op_array->type)) {
		return 0;
	}
	if (CG(compiler_options) & ZEND_COMPILE_EXTENDED_INFO) {
		zend_update_extended_info(op_array);
	}
	if (CG(compiler_options) & ZEND_COMPILE_HANDLE_OP_ARRAY) {
		if (zend_extension_flags & ZEND_EXTENSIONS_HAVE_OP_ARRAY_HANDLER) {
			zend_llist_apply_with_argument(&zend_extensions, (llist_apply_with_arg_func_t) zend_extension_op_array_handler, op_array);
		}
	}

	if (CG(context).vars_size != op_array->last_var) {
		op_array->vars = (zend_string**) erealloc(op_array->vars, sizeof(zend_string*)*op_array->last_var);
		CG(context).vars_size = op_array->last_var;
	}

#if ZEND_USE_ABS_CONST_ADDR
	if (CG(context).opcodes_size != op_array->last) {
		op_array->opcodes = (zend_op *) erealloc(op_array->opcodes, sizeof(zend_op)*op_array->last);
		CG(context).opcodes_size = op_array->last;
	}
	if (CG(context).literals_size != op_array->last_literal) {
		op_array->literals = (zval*)erealloc(op_array->literals, sizeof(zval) * op_array->last_literal);
		CG(context).literals_size = op_array->last_literal;
	}
#else
	op_array->opcodes = (zend_op *) erealloc(op_array->opcodes,
		ZEND_MM_ALIGNED_SIZE_EX(sizeof(zend_op) * op_array->last, 16) +
		sizeof(zval) * op_array->last_literal);
	if (op_array->literals) {
		memcpy(((char*)op_array->opcodes) + ZEND_MM_ALIGNED_SIZE_EX(sizeof(zend_op) * op_array->last, 16),
			op_array->literals, sizeof(zval) * op_array->last_literal);
		efree(op_array->literals);
		op_array->literals = (zval*)(((char*)op_array->opcodes) + ZEND_MM_ALIGNED_SIZE_EX(sizeof(zend_op) * op_array->last, 16));
	}
	CG(context).opcodes_size = op_array->last;
	CG(context).literals_size = op_array->last_literal;
#endif

	/* Needs to be set directly after the opcode/literal reallocation, to ensure destruction
	 * happens correctly if any of the following fixups generate a fatal error. */
	op_array->fn_flags |= ZEND_ACC_DONE_PASS_TWO;

	opline = op_array->opcodes;
	end = opline + op_array->last;
	while (opline < end) {
		switch (opline->opcode) {
			case ZEND_RECV_INIT:
				{
					zval *val = CT_CONSTANT(opline->op2);
					if (Z_TYPE_P(val) == IS_CONSTANT_AST) {
						uint32_t slot = ZEND_MM_ALIGNED_SIZE_EX(op_array->cache_size, 8);
						Z_CACHE_SLOT_P(val) = slot;
						op_array->cache_size += sizeof(zval);
					}
				}
				break;
			case ZEND_FAST_CALL:
				opline->op1.opline_num = op_array->try_catch_array[opline->op1.num].finally_op;
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op1);
				break;
			case ZEND_BRK:
			case ZEND_CONT:
				{
					uint32_t jmp_target = zend_get_brk_cont_target(op_array, opline);

					if (op_array->fn_flags & ZEND_ACC_HAS_FINALLY_BLOCK) {
						zend_check_finally_breakout(op_array, opline - op_array->opcodes, jmp_target);
					}
					opline->opcode = ZEND_JMP;
					opline->op1.opline_num = jmp_target;
					opline->op2.num = 0;
					ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op1);
				}
				break;
			case ZEND_GOTO:
				zend_resolve_goto_label(op_array, opline);
				if (op_array->fn_flags & ZEND_ACC_HAS_FINALLY_BLOCK) {
					zend_check_finally_breakout(op_array, opline - op_array->opcodes, opline->op1.opline_num);
				}
				/* break omitted intentionally */
			case ZEND_JMP:
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op1);
				break;
			case ZEND_JMPZNZ:
				/* absolute index to relative offset */
				opline->extended_value = ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, opline->extended_value);
				/* break omitted intentionally */
			case ZEND_JMPZ:
			case ZEND_JMPNZ:
			case ZEND_JMPZ_EX:
			case ZEND_JMPNZ_EX:
			case ZEND_JMP_SET:
			case ZEND_COALESCE:
			case ZEND_FE_RESET_R:
			case ZEND_FE_RESET_RW:
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op2);
				break;
			case ZEND_ASSERT_CHECK:
			{
				/* If result of assert is unused, result of check is unused as well */
				zend_op *call = &op_array->opcodes[opline->op2.opline_num - 1];
				if (call->opcode == ZEND_EXT_FCALL_END) {
					call--;
				}
				if (call->result_type == IS_UNUSED) {
					opline->result_type = IS_UNUSED;
				}
				ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op2);
				break;
			}
			case ZEND_DECLARE_ANON_CLASS:
			case ZEND_DECLARE_ANON_INHERITED_CLASS:
			case ZEND_FE_FETCH_R:
			case ZEND_FE_FETCH_RW:
				/* absolute index to relative offset */
				opline->extended_value = ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, opline->extended_value);
				break;
			case ZEND_CATCH:
				if (!(opline->extended_value & ZEND_LAST_CATCH)) {
					ZEND_PASS_TWO_UPDATE_JMP_TARGET(op_array, opline, opline->op2);
				}
				break;
			case ZEND_RETURN:
			case ZEND_RETURN_BY_REF:
				if (op_array->fn_flags & ZEND_ACC_GENERATOR) {
					opline->opcode = ZEND_GENERATOR_RETURN;
				}
				break;
			case ZEND_SWITCH_LONG:
			case ZEND_SWITCH_STRING:
			{
				/* absolute indexes to relative offsets */
				HashTable *jumptable = Z_ARRVAL_P(CT_CONSTANT(opline->op2));
				zval *zv;
				ZEND_HASH_FOREACH_VAL(jumptable, zv) {
					Z_LVAL_P(zv) = ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, Z_LVAL_P(zv));
				} ZEND_HASH_FOREACH_END();

				opline->extended_value = ZEND_OPLINE_NUM_TO_OFFSET(op_array, opline, opline->extended_value);
				break;
			}
		}
		if (opline->op1_type == IS_CONST) {
			ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, opline, opline->op1);
		} else if (opline->op1_type & (IS_VAR|IS_TMP_VAR)) {
			opline->op1.var = (uint32_t)(zend_intptr_t)ZEND_CALL_VAR_NUM(NULL, op_array->last_var + opline->op1.var);
		}
		if (opline->op2_type == IS_CONST) {
			ZEND_PASS_TWO_UPDATE_CONSTANT(op_array, opline, opline->op2);
		} else if (opline->op2_type & (IS_VAR|IS_TMP_VAR)) {
			opline->op2.var = (uint32_t)(zend_intptr_t)ZEND_CALL_VAR_NUM(NULL, op_array->last_var + opline->op2.var);
		}
		if (opline->result_type & (IS_VAR|IS_TMP_VAR)) {
			opline->result.var = (uint32_t)(zend_intptr_t)ZEND_CALL_VAR_NUM(NULL, op_array->last_var + opline->result.var);
		}
		ZEND_VM_SET_OPCODE_HANDLER(opline);
		opline++;
	}

	if (op_array->live_range) {
		int i;

		zend_sort_live_ranges(op_array);
		for (i = 0; i < op_array->last_live_range; i++) {
			op_array->live_range[i].var =
				(uint32_t)(zend_intptr_t)ZEND_CALL_VAR_NUM(NULL, op_array->last_var + (op_array->live_range[i].var / sizeof(zval))) |
				(op_array->live_range[i].var & ZEND_LIVE_MASK);
		}
	}

	return 0;
}

```

这个函数主要作用是设置opcode对应的handle，我们看一个关键宏函数ZEND_VM_SET_OPCODE_HANDLER(opline);

```
#define ZEND_VM_SET_OPCODE_HANDLER(opline) zend_vm_set_opcode_handler(opline)
```
分享到这里我们可以看一下秦明老师的github把之前语法解析 到 组成opcode的知识点串起来了，具体看github地址

https://github.com/LeiZhang-Hunter/php7-internal/blob/master/3/zend_compile_opcode.md
