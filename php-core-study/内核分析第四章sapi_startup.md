我在这里看到了

sapi_startup

	sapi_startup(&cgi_sapi_module);
	
进去继续看源代码

我们首先要看一下sapi_module_struct这个结构体是什么样子的

```
struct _sapi_module_struct {
	char *name;
	char *pretty_name;

	int (*startup)(struct _sapi_module_struct *sapi_module);
	int (*shutdown)(struct _sapi_module_struct *sapi_module);

	int (*activate)(void);
	int (*deactivate)(void);

	size_t (*ub_write)(const char *str, size_t str_length);
	void (*flush)(void *server_context);
	zend_stat_t *(*get_stat)(void);
	char *(*getenv)(char *name, size_t name_len);

	void (*sapi_error)(int type, const char *error_msg, ...) ZEND_ATTRIBUTE_FORMAT(printf, 2, 3);

	int (*header_handler)(sapi_header_struct *sapi_header, sapi_header_op_enum op, sapi_headers_struct *sapi_headers);
	int (*send_headers)(sapi_headers_struct *sapi_headers);
	void (*send_header)(sapi_header_struct *sapi_header, void *server_context);

	size_t (*read_post)(char *buffer, size_t count_bytes);
	char *(*read_cookies)(void);

	void (*register_server_variables)(zval *track_vars_array);
	void (*log_message)(char *message, int syslog_type_int);
	double (*get_request_time)(void);
	void (*terminate_process)(void);

	char *php_ini_path_override;

	void (*default_post_reader)(void);
	void (*treat_data)(int arg, char *str, zval *destArray);
	char *executable_location;

	int php_ini_ignore;
	int php_ini_ignore_cwd; /* don't look for php.ini in the current directory */

	int (*get_fd)(int *fd);

	int (*force_http_10)(void);

	int (*get_target_uid)(uid_t *);
	int (*get_target_gid)(gid_t *);

	unsigned int (*input_filter)(int arg, char *var, char **val, size_t val_len, size_t *new_val_len);

	void (*ini_defaults)(HashTable *configuration_hash);
	int phpinfo_as_text;

	char *ini_entries;
	const zend_function_entry *additional_functions;
	unsigned int (*input_filter_init)(void);
};
```

我们发现这里面初始化了各种各样的函数地址

	SAPI_API sapi_module_struct sapi_module;
	
这里定义了一个全局的sapi_model变量

	sf->ini_entries = NULL;
	sapi_module = *sf;
	
然后我们再次回头查看调用位置

	sapi_startup(&cgi_sapi_module);
	
再回头看这个cgi_sapi_module，发现它定义了一个全局数组，这样就完成了对sapi_module的赋值

```
static sapi_module_struct cgi_sapi_module = {
	"cgi-fcgi",						/* name */
	"CGI/FastCGI",					/* pretty name */

	php_cgi_startup,				/* startup */
	php_module_shutdown_wrapper,	/* shutdown */

	sapi_cgi_activate,				/* activate */
	sapi_cgi_deactivate,			/* deactivate */

	sapi_cgi_ub_write,				/* unbuffered write */
	sapi_cgi_flush,					/* flush */
	NULL,							/* get uid */
	sapi_cgi_getenv,				/* getenv */

	php_error,						/* error handler */

	NULL,							/* header handler */
	sapi_cgi_send_headers,			/* send headers handler */
	NULL,							/* send header handler */

	sapi_cgi_read_post,				/* read POST data */
	sapi_cgi_read_cookies,			/* read Cookies */

	sapi_cgi_register_variables,	/* register server variables */
	sapi_cgi_log_message,			/* Log message */
	NULL,							/* Get request time */
	NULL,							/* Child terminate */

	STANDARD_SAPI_MODULE_PROPERTIES
};
```
	
这里进行了初始化赋值,有各种各样的sapi模块的函数，然后我们继续看sapi_startup

```
SAPI_API void sapi_startup(sapi_module_struct *sf)
{
	sf->ini_entries = NULL;
	sapi_module = *sf;

#ifdef ZTS
	ts_allocate_id(&sapi_globals_id, sizeof(sapi_globals_struct), (ts_allocate_ctor) sapi_globals_ctor, (ts_allocate_dtor) sapi_globals_dtor);
# ifdef PHP_WIN32
	_configthreadlocale(_ENABLE_PER_THREAD_LOCALE);
# endif
#else
	sapi_globals_ctor(&sapi_globals);
#endif

#ifdef PHP_WIN32
	tsrm_win32_startup();
#endif

	reentrancy_startup();
}
```

老规矩只看非线程安全的，第一个核心函数出现了

	sapi_globals_ctor(&sapi_globals);
	
我们看到了源代码

	static void sapi_globals_ctor(sapi_globals_struct *sapi_globals)
	{
	#ifdef ZTS
		ZEND_TSRMLS_CACHE_UPDATE();
	#endif
		memset(sapi_globals, 0, sizeof(*sapi_globals));
		zend_hash_init_ex(&sapi_globals->known_post_content_types, 8, NULL, _type_dtor, 1, 0);
		php_setup_sapi_content_types();
	}
	
看到了这里我想到要看一下sapi_globals_struct这个结构体的样貌

	typedef struct _sapi_globals_struct {
		void *server_context;
		sapi_request_info request_info;
		sapi_headers_struct sapi_headers;
		int64_t read_post_bytes;
		unsigned char post_read;
		unsigned char headers_sent;
		zend_stat_t global_stat;
		char *default_mimetype;
		char *default_charset;
		HashTable *rfc1867_uploaded_files;
		zend_long post_max_size;
		int options;
		zend_bool sapi_started;
		double global_request_time;
		HashTable known_post_content_types;
		zval callback_func;
		zend_fcall_info_cache fci_cache;
	} sapi_globals_struct;
	
我们继续看

	memset(sapi_globals, 0, sizeof(*sapi_globals));
	
跟bzero一样初始化这个结构体

	zend_hash_init_ex(&sapi_globals->known_post_content_types, 8, NULL, _type_dtor, 1, 0);
	
这个上一节分析过 初始化一个8个字节的Hashtable给known_post_content_types，在继续看php_setup_sapi_content_types();

```
int php_setup_sapi_content_types(void)
{
	sapi_register_post_entries(php_post_entries);

	return SUCCESS;
}
```

到这里我们看到了第一个变量php_post_entries

```
struct _sapi_post_entry {
	char *content_type;
	uint32_t content_type_len;
	void (*post_reader)(void);
	void (*post_handler)(char *content_type_dup, void *arg);
};

```


然后我们在继续查看sapi_register_post_entries我们看看他到底做了什么

```
SAPI_API int sapi_register_post_entries(const sapi_post_entry *post_entries)
{
	const sapi_post_entry *p=post_entries;

	while (p->content_type) {
		if (sapi_register_post_entry(p) == FAILURE) {
			return FAILURE;
		}
		p++;
	}
	return SUCCESS;
}
```

如果说content_type是有值的那么我们就可以继续进行下一步，执行sapi_register_post_entry,到这里我们要做一下总结，由参数我们可以知道进来的可一个数组地址，把我们之前分析的所有内容做一次画图分析

```
SAPI_API int sapi_register_post_entry(const sapi_post_entry *post_entry)
{
	int ret;
	zend_string *key;
	if (SG(sapi_started) && EG(current_execute_data)) {
		return FAILURE;
	}
	key = zend_string_init(post_entry->content_type, post_entry->content_type_len, 1);
	GC_MAKE_PERSISTENT_LOCAL(key);
	ret = zend_hash_add_mem(&SG(known_post_content_types), key,
			(void *) post_entry, sizeof(sapi_post_entry)) ? SUCCESS : FAILURE;
	zend_string_release_ex(key, 1);
	return ret;
}
```
这里我们首先看两个宏SG和EG

```
# define SG(v) (sapi_globals.v)
extern SAPI_API sapi_globals_struct sapi_globals;
```
sapi_globals_struct这个我们在上面已经做出了分析 这里只是去除sapi_started，这个zend_bool 变量 

zend_bool 变量的定义

```
typedef unsigned char zend_bool;
typedef unsigned char zend_uchar;
```

我们再看一下EG这个变量的定义

```
#ifdef ZTS
# define EG(v) ZEND_TSRMG(executor_globals_id, zend_executor_globals *, v)
#else
# define EG(v) (executor_globals.v)
extern ZEND_API zend_executor_globals executor_globals;
#endif
```

在这里我看到了一个结构体
```
struct _zend_executor_globals {
	zval uninitialized_zval;
	zval error_zval;

	/* symbol table cache */
	zend_array *symtable_cache[SYMTABLE_CACHE_SIZE];
	zend_array **symtable_cache_limit;
	zend_array **symtable_cache_ptr;

	zend_array symbol_table;		/* main symbol table */

	HashTable included_files;	/* files already included */

	JMP_BUF *bailout;

	int error_reporting;
	int exit_status;

	HashTable *function_table;	/* function symbol table */
	HashTable *class_table;		/* class table */
	HashTable *zend_constants;	/* constants table */

	zval          *vm_stack_top;
	zval          *vm_stack_end;
	zend_vm_stack  vm_stack;
	size_t         vm_stack_page_size;

	struct _zend_execute_data *current_execute_data;
	zend_class_entry *fake_scope; /* used to avoid checks accessing properties */

	zend_long precision;

	int ticks_count;

	uint32_t persistent_constants_count;
	uint32_t persistent_functions_count;
	uint32_t persistent_classes_count;

	HashTable *in_autoload;
	zend_function *autoload_func;
	zend_bool full_tables_cleanup;

	/* for extended information support */
	zend_bool no_extensions;

	zend_bool vm_interrupt;
	zend_bool timed_out;
	zend_long hard_timeout;

#ifdef ZEND_WIN32
	OSVERSIONINFOEX windows_version_info;
#endif

	HashTable regular_list;
	HashTable persistent_list;

	int user_error_handler_error_reporting;
	zval user_error_handler;
	zval user_exception_handler;
	zend_stack user_error_handlers_error_reporting;
	zend_stack user_error_handlers;
	zend_stack user_exception_handlers;

	zend_error_handling_t  error_handling;
	zend_class_entry      *exception_class;

	/* timeout support */
	zend_long timeout_seconds;

	int lambda_count;

	HashTable *ini_directives;
	HashTable *modified_ini_directives;
	zend_ini_entry *error_reporting_ini_entry;

	zend_objects_store objects_store;
	zend_object *exception, *prev_exception;
	const zend_op *opline_before_exception;
	zend_op exception_op[3];

	struct _zend_module_entry *current_module;

	zend_bool active;
	zend_uchar flags;

	zend_long assertions;

	uint32_t           ht_iterators_count;     /* number of allocatd slots */
	uint32_t           ht_iterators_used;      /* number of used slots */
	HashTableIterator *ht_iterators;
	HashTableIterator  ht_iterators_slots[16];

	void *saved_fpu_cw_ptr;
#if XPFPA_HAVE_CW
	XPFPA_CW_DATATYPE saved_fpu_cw;
#endif

	zend_function trampoline;
	zend_op       call_trampoline_op;

	zend_bool each_deprecation_thrown;

	void *reserved[ZEND_MAX_RESERVED_RESOURCES];
};

typedef struct _zend_executor_globals zend_executor_globals;
```

我不得不说这真的是一个十分复杂的结构体啊，但是我们看这里

	EG(current_execute_data)
	
在这里他是对结构体中的这一个值做了判断

```
struct _zend_execute_data *current_execute_data;
```

这里我看到了_zend_execute_data这个结构体,我们继续看一下这个结构体的定义

```
struct _zend_execute_data {
	const zend_op       *opline;           /* executed opline                */
	zend_execute_data   *call;             /* current call                   */
	zval                *return_value;
	zend_function       *func;             /* executed function              */
	zval                 This;             /* this + call_info + num_args    */
	zend_execute_data   *prev_execute_data;
	zend_array          *symbol_table;
#if ZEND_EX_USE_RUN_TIME_CACHE
	void               **run_time_cache;   /* cache op_array->run_time_cache */
#endif
};
```
说道这个变量我想到了我每次写扩展中的_zend_execute_data，PHP_METHOD总是挂着这个参数，这里不做具体分析据需带着疑惑和不解向下读代码

这里说实话我也不能准确定义出每个变量地址是做什么的

	if (SG(sapi_started) && EG(current_execute_data)) {
		return FAILURE;
	}

但是在这里我们可以但从代码层面上理解就是如果sapi 已经启动了，或者current_execute_data不是空的 那么我们就返回FAILURE,FAILURE的宏定义

	typedef enum {
	  SUCCESS =  0,
	  FAILURE = -1,		/* this MUST stay a negative number, or it may affect functions! */
	} ZEND_RESULT_CODE;
	
我在继续向下看源代码我看了这一行，这是我写扩展经常用到的一行

	key = zend_string_init(post_entry->content_type, post_entry->content_type_len, 1);
	
今天我也认为我有必要深入看一下他到底是什么

	static zend_always_inline zend_string *zend_string_init(const char *str, size_t len, int persistent)
	{
		zend_string *ret = zend_string_alloc(len, persistent);

		memcpy(ZSTR_VAL(ret), str, len);
		ZSTR_VAL(ret)[len] = '\0';
		return ret;
	}
	
第一个字符串是char一个连续的地址，第二个是len，这其实就是“unix网络编程第一卷”的3.3章节的值和结果参数,第三个参数 跟 zend_hash_init参数里面的 persistent一样好像是一个记录持久化的参数，以后再慢慢看吧。不多说我在这里又看到了zend_string_alloc这个函数

	static zend_always_inline zend_string *zend_string_alloc(size_t len, int persistent)
	{
		zend_string *ret = (zend_string *)pemalloc(ZEND_MM_ALIGNED_SIZE(_ZSTR_STRUCT_SIZE(len)), persistent);

		GC_SET_REFCOUNT(ret, 1);
		GC_TYPE_INFO(ret) = IS_STRING | ((persistent ? IS_STR_PERSISTENT : 0) << GC_FLAGS_SHIFT);
		zend_string_forget_hash_val(ret);
		ZSTR_LEN(ret) = len;
		return ret;
	}

到了这个函数里 我们首先要看 一下 pemalloc 这个函数的实现

	#define pemalloc(size, persistent) ((persistent)?__zend_malloc(size):emalloc(size))
	
是否是持久化？如果是持久化则调用__zend_malloc，如果不是持久化则调用emalloc，那么我们要进一步追踪\_\_zend_malloc和emalloc的区别了，我们先看\_\_zend_malloc这个函数

	ZEND_API void * __zend_malloc(size_t len)
	{
		void *tmp = malloc(len);
		if (EXPECTED(tmp || !len)) {
			return tmp;
		}
		zend_out_of_memory();
	}

如果说malloc 成功或者长度为0，那么直接返回tmp，malloc的地址，如果说tmp为空，也就是说malloc失败

	static ZEND_COLD ZEND_NORETURN void zend_out_of_memory(void)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

直接调用这个函数exit掉返回

那么我们再次看一下emalloc做了什么

	/* Standard wrapper macros */
	#define emalloc(size)						_emalloc((size) ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)
	
看到了这里我在想 ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC 是什么

先看ZEND_FILE_LINE_CC吧

	# define ZEND_FILE_LINE_CC
	
	# define ZEND_FILE_LINE_EMPTY_CC
	
可以看到都是空的，我认为这里的寓意更像是一个空的占位符，我们写一个demo看一下

	#include <sys/un.h>
	#include <stdio.h>
	#include <sys/socket.h>
	#include <unistd.h>
	#include <stdio.h>
	#include <malloc.h>

	# define ZEND_FILE_LINE_CC

	# define ZEND_FILE_LINE_EMPTY_CC

	#define emalloc(size) malloc((size)ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC)

	int main() {
	    void* add = emalloc(20);
	    printf("%p\n",add);
	}

和malloc没有任何区别，那么我接下来看一下_emalloc到底做了什么

```
ZEND_API void* ZEND_FASTCALL _emalloc(size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
#if ZEND_MM_CUSTOM
	if (UNEXPECTED(AG(mm_heap)->use_custom_heap)) {
		if (ZEND_DEBUG && AG(mm_heap)->use_custom_heap == ZEND_MM_CUSTOM_HEAP_DEBUG) {
			return AG(mm_heap)->custom_heap.debug._malloc(size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
		} else {
			return AG(mm_heap)->custom_heap.std._malloc(size);
		}
	}
#endif
	return zend_mm_alloc_heap(AG(mm_heap), size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
}
```

在这里我看到了一些宏定义，开始第一个令我比较疑惑的就是AG这个宏定义,于是我开始查找代码中的这个宏定义，我发现了

```
#ifdef ZTS
static int alloc_globals_id;
# define AG(v) ZEND_TSRMG(alloc_globals_id, zend_alloc_globals *, v)
#else
# define AG(v) (alloc_globals.v)
static zend_alloc_globals alloc_globals;
#endif
```

这个宏定义,在这里定义了，但是这个结构体zend_alloc_globals到底是什么呢

```
typedef struct _zend_alloc_globals {
	zend_mm_heap *mm_heap;
} zend_alloc_globals;
```

读到这里我又思考zend_mm_heap,是什么，通过这个定义我觉得像zend内存堆，又开始看zend_mm_heap这个结构体是什么，看到了这里

```
typedef struct _zend_mm_heap zend_mm_heap;
```
这里是把struct _zend_mm_heap重新定义了，再继续往下看，哇！！看到了好大一个结构体啊

```
struct _zend_mm_heap {
#if ZEND_MM_CUSTOM
	int                use_custom_heap;
#endif
#if ZEND_MM_STORAGE
	zend_mm_storage   *storage;
#endif
#if ZEND_MM_STAT
	size_t             size;                    /* current memory usage */
	size_t             peak;                    /* peak memory usage */
#endif
	zend_mm_free_slot *free_slot[ZEND_MM_BINS]; /* free lists for small sizes */
#if ZEND_MM_STAT || ZEND_MM_LIMIT
	size_t             real_size;               /* current size of allocated pages */
#endif
#if ZEND_MM_STAT
	size_t             real_peak;               /* peak size of allocated pages */
#endif
#if ZEND_MM_LIMIT
	size_t             limit;                   /* memory limit */
	int                overflow;                /* memory overflow flag */
#endif

	zend_mm_huge_list *huge_list;               /* list of huge allocated blocks */

	zend_mm_chunk     *main_chunk;
	zend_mm_chunk     *cached_chunks;			/* list of unused chunks */
	int                chunks_count;			/* number of alocated chunks */
	int                peak_chunks_count;		/* peak number of allocated chunks for current request */
	int                cached_chunks_count;		/* number of cached chunks */
	double             avg_chunks_count;		/* average number of chunks allocated per request */
	int                last_chunks_delete_boundary; /* numer of chunks after last deletion */
	int                last_chunks_delete_count;    /* number of deletion over the last boundary */
#if ZEND_MM_CUSTOM
	union {
		struct {
			void      *(*_malloc)(size_t);
			void       (*_free)(void*);
			void      *(*_realloc)(void*, size_t);
		} std;
		struct {
			void      *(*_malloc)(size_t ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
			void       (*_free)(void*  ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
			void      *(*_realloc)(void*, size_t  ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
		} debug;
	} custom_heap;
#endif
};
```

看到了定义了好多我不知道的宏定义，这怎么办呢，我使用gdb调试一把！！！



哈哈发现进到了ZEND_MM_CUSTOM里，ZEND_MM_CUSTOM在这里得到了定义

```
#ifndef ZEND_MM_CUSTOM
# define ZEND_MM_CUSTOM 1  /* support for custom memory allocator            */
                           /* USE_ZEND_ALLOC=0 may switch to system malloc() */
#endif
```

我们使用gdb查看一下alloc_globals这个结构体

```
(gdb) print alloc_globals
$1 = {mm_heap = 0x7fffeea00040}
(gdb) print alloc_globals->mm_heap
$2 = (zend_mm_heap *) 0x7fffeea00040
(gdb) print *alloc_globals->mm_heap
$3 = {use_custom_heap = 0, storage = 0x0, size = 20520, peak = 20520, free_slot = {0x7fffeea02000, 0x0, 0x0, 0x0, 0x7fffeea01028, 0x7fffeea03000, 0x0 <repeats 24 times>}, real_size = 2097152, 
  real_peak = 2097152, limit = 9223372036854775807, overflow = 0, huge_list = 0x0, main_chunk = 0x7fffeea00000, cached_chunks = 0x0, chunks_count = 1, peak_chunks_count = 1, cached_chunks_count = 0, 
  avg_chunks_count = 1, last_chunks_delete_boundary = 0, last_chunks_delete_count = 0, custom_heap = {std = {_malloc = 0x0, _free = 0x0, _realloc = 0x0}, debug = {_malloc = 0x0, _free = 0x0, 
      _realloc = 0x0}}}
(gdb) 
```
这段代码的意思是如果说AG(mm_heap)->use_custom_heap有很大概率不会进入if条件，然后如果是在调试模式执行

```
return AG(mm_heap)->custom_heap.debug._malloc(size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
```

在这里我们直接查看非调试模式下的吧

```
return AG(mm_heap)->custom_heap.std._malloc(size);
```

这里我有了一个疑问点，就是alloc_globals这个结构体在我分析源码至今好像都没初始化过，到底是在哪里初始化的呢，在什么时候初始化的结构体中这些匿名的联合体的函数地址呢？

用gdb调试之后发现并没有被初始化

```
(gdb) print *alloc_globals->mm_heap->use_custom_heap
Cannot access memory at address 0x0
```

我发现这段程序并没有走进去，一直在2497这行，然后直接跳到2505这一行

写了一个demo

```
# define UNEXPECTED(condition) __builtin_expect(!!(condition), 0)
int main()
{
    if(UNEXPECTED(0))
    {
        printf("111\n");
    }
}
```

发现并没有进去，验证了很大概率进不去if条件这种情况，所以这段程序直接走的

```
return zend_mm_alloc_heap(AG(mm_heap), size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
```

继续跟进查看一下这个函数到底做了什么

```
static zend_always_inline void *zend_mm_alloc_heap(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
	void *ptr;
#if ZEND_DEBUG
	size_t real_size = size;
	zend_mm_debug_info *dbg;

	/* special handling for zero-size allocation */
	size = MAX(size, 1);
	size = ZEND_MM_ALIGNED_SIZE(size) + ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info));
	if (UNEXPECTED(size < real_size)) {
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%zu + %zu)", ZEND_MM_ALIGNED_SIZE(real_size), ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
		return NULL;
	}
#endif
	if (EXPECTED(size <= ZEND_MM_MAX_SMALL_SIZE)) {
		ptr = zend_mm_alloc_small(heap, size, ZEND_MM_SMALL_SIZE_TO_BIN(size) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#if ZEND_DEBUG
		dbg = zend_mm_get_debug_info(heap, ptr);
		dbg->size = real_size;
		dbg->filename = __zend_filename;
		dbg->orig_filename = __zend_orig_filename;
		dbg->lineno = __zend_lineno;
		dbg->orig_lineno = __zend_orig_lineno;
#endif
		return ptr;
	} else if (EXPECTED(size <= ZEND_MM_MAX_LARGE_SIZE)) {
		ptr = zend_mm_alloc_large(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#if ZEND_DEBUG
		dbg = zend_mm_get_debug_info(heap, ptr);
		dbg->size = real_size;
		dbg->filename = __zend_filename;
		dbg->orig_filename = __zend_orig_filename;
		dbg->lineno = __zend_lineno;
		dbg->orig_lineno = __zend_orig_lineno;
#endif
		return ptr;
	} else {
#if ZEND_DEBUG
		size = real_size;
#endif
		return zend_mm_alloc_huge(heap, size ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	}
}
```

从这一段代码中我看到如果说字节数小于3072，就会走zend_mm_alloc_small这个分支



这里防止malloc 为 0

```
/* special handling for zero-size allocation */
	size = MAX(size, 1);
```

我们继续向下看代码

```
	size = ZEND_MM_ALIGNED_SIZE(size) + ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info));

```

看到这里我思考ZEND_MM_ALIGNED_SIZE这个宏是做什么的？

```
#define ZEND_MM_ALIGNED_SIZE(size)	(((size) + ZEND_MM_ALIGNMENT - Z_L(1)) & ZEND_MM_ALIGNMENT_MASK)
```

看到这里我们又需要看 

```
# define ZEND_MM_ALIGNMENT Z_L(8)
```

这个宏他做了什么，Z_L(8) 这个是什么呢

```
# define Z_L(i) INT32_C(i)

#ifndef INT32_C
# define INT32_C(c) c
#endif
```

我们再看ZEND_MM_ALIGNMENT_MASK 这个宏 是什么

```
#define ZEND_MM_ALIGNMENT_MASK ~(ZEND_MM_ALIGNMENT - Z_L(1))
```
现在所有的宏都给出了定义但是我不明白他进行字节对其为什么要这么做，那么我们现在再写一个demo，进行分析

```
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

# define Z_L(i) INT32_C(i)

#ifndef INT32_C
# define INT32_C(c) c
#endif

# define ZEND_MM_ALIGNMENT Z_L(8)

#define ZEND_MM_ALIGNMENT_MASK ~(ZEND_MM_ALIGNMENT - Z_L(1))

#define ZEND_MM_ALIGNED_SIZE(size)	(((size) + ZEND_MM_ALIGNMENT - Z_L(1)) & ZEND_MM_ALIGNMENT_MASK)

int main()
{
    printf("%d\n",ZEND_MM_ALIGNED_SIZE(333));
    return 0;
}
```

当我输入1000的时候 printf是1000时候结果为1000，但是当我们输入333的时候结果是336，所以说他会把size变为8的倍数的数字，剩下的代码就比较容易理解了

```
if (UNEXPECTED(size < real_size)) {
		zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%zu + %zu)", ZEND_MM_ALIGNED_SIZE(real_size), ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
		return NULL;
	}
```

如果说size比自己真实的数字小，也就意味着溢出了，提醒报错，申请内存的尺寸溢出了，后面的逻辑很简单了

```
if (EXPECTED(size <= ZEND_MM_MAX_SMALL_SIZE)) {
		ptr = zend_mm_alloc_small(heap, size, ZEND_MM_SMALL_SIZE_TO_BIN(size) ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
```

如果说小于小尺寸的内存分配，ZEND_MM_MAX_SMALL_SIZE为3072，那么就申请一个小尺寸的堆，我们看一下zend_mm_alloc_small的实现

```
static zend_always_inline void *zend_mm_alloc_small(zend_mm_heap *heap, size_t size, int bin_num ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
#if ZEND_MM_STAT
	do {
		size_t size = heap->size + bin_data_size[bin_num];
		size_t peak = MAX(heap->peak, size);
		heap->size = size;
		heap->peak = peak;
	} while (0);
#endif

	if (EXPECTED(heap->free_slot[bin_num] != NULL)) {
		zend_mm_free_slot *p = heap->free_slot[bin_num];
		heap->free_slot[bin_num] = p->next_free_slot;
		return (void*)p;
	} else {
		return zend_mm_alloc_small_slow(heap, bin_num ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	}
}
```

我使用gdb 查看了heap的内存分配情况

```
$8 = {use_custom_heap = 0, storage = 0x0, size = 48, peak = 48, free_slot = {
    0x0 <repeats 30 times>}, real_size = 2097152, real_peak = 2097152, 
  limit = 9223372036854775807, overflow = 0, huge_list = 0x0, 
  main_chunk = 0x7fffeea00000, cached_chunks = 0x0, chunks_count = 1, 
  peak_chunks_count = 1, cached_chunks_count = 0, avg_chunks_count = 1, 
  last_chunks_delete_boundary = 0, last_chunks_delete_count = 0, 
  custom_heap = {std = {_malloc = 0x0, _free = 0x0, _realloc = 0x0}, debug = {
      _malloc = 0x0, _free = 0x0, _realloc = 0x0}}}

```

我们看到

```
	if (EXPECTED(heap->free_slot[bin_num] != NULL)) {
		zend_mm_free_slot *p = heap->free_slot[bin_num];
		heap->free_slot[bin_num] = p->next_free_slot;
		return (void*)p;
	} else {
		return zend_mm_alloc_small_slow(heap, bin_num ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
	}
```

这里通过判断free_slot 是不是为空的，如果不是空的，但是我们需要注意一下bin_num这个参数，我们就要再回到传参的位置

```
#define ZEND_MM_SMALL_SIZE_TO_BIN(size)  zend_mm_small_size_to_bin(size)
```

看到这里我们就要看zend_mm_small_size_to_bin这个函数是做什么用的

```
static zend_always_inline int zend_mm_small_size_to_bin(size_t size)
{
#if 0
	int n;
                            /*0,  1,  2,  3,  4,  5,  6,  7,  8,  9  10, 11, 12*/
	static const int f1[] = { 3,  3,  3,  3,  3,  3,  3,  4,  5,  6,  7,  8,  9};
	static const int f2[] = { 0,  0,  0,  0,  0,  0,  0,  4,  8, 12, 16, 20, 24};

	if (UNEXPECTED(size <= 2)) return 0;
	n = zend_mm_small_size_to_bit(size - 1);
	return ((size-1) >> f1[n]) + f2[n];
#else
	unsigned int t1, t2;

	if (size <= 64) {
		/* we need to support size == 0 ... */
		return (size - !!size) >> 3;
	} else {
		t1 = size - 1;
		t2 = zend_mm_small_size_to_bit(t1) - 3;
		t1 = t1 >> t2;
		t2 = t2 - 3;
		t2 = t2 << 2;
		return (int)(t1 + t2);
	}
#endif
}
```

在这里给出了函数的实现，我们看到还有一个zend_mm_small_size_to_bit这个函数，我们再继续看这个函数

```
/* higher set bit number (0->N/A, 1->1, 2->2, 4->3, 8->4, 127->7, 128->8 etc) */
static zend_always_inline int zend_mm_small_size_to_bit(int size)
{
#if (defined(__GNUC__) || __has_builtin(__builtin_clz))  && defined(PHP_HAVE_BUILTIN_CLZ)
	return (__builtin_clz(size) ^ 0x1f) + 1;
#elif defined(_WIN32)
	unsigned long index;

	if (!BitScanReverse(&index, (unsigned long)size)) {
		/* undefined behavior */
		return 64;
	}

	return (((31 - (int)index) ^ 0x1f) + 1);
#else
	int n = 16;
	if (size <= 0x00ff) {n -= 8; size = size << 8;}
	if (size <= 0x0fff) {n -= 4; size = size << 4;}
	if (size <= 0x3fff) {n -= 2; size = size << 2;}
	if (size <= 0x7fff) {n -= 1;}
	return n;
#endif
}
```

谢了一个简单的demo

```
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

# define Z_L(i) INT32_C(i)

#ifndef INT32_C
# define INT32_C(c) c
#endif

# define ZEND_MM_ALIGNMENT Z_L(8)

#define ZEND_MM_ALIGNMENT_MASK ~(ZEND_MM_ALIGNMENT - Z_L(1))
#define ZEND_MM_STAT
#define ZEND_MM_ALIGNED_SIZE(size)	(((size) + ZEND_MM_ALIGNMENT - Z_L(1)) & ZEND_MM_ALIGNMENT_MASK)

/* higher set bit number (0->N/A, 1->1, 2->2, 4->3, 8->4, 127->7, 128->8 etc) */
static inline int zend_mm_small_size_to_bit(int size)
{

    int n = 16;
    if (size <= 0x00ff) {n -= 8; size = size << 8;}
    if (size <= 0x0fff) {n -= 4; size = size << 4;}
    if (size <= 0x3fff) {n -= 2; size = size << 2;}
    if (size <= 0x7fff) {n -= 1;}
    return n;
}

static inline int zend_mm_small_size_to_bin(size_t size)
{
#if 0
    int n;
                            /*0,  1,  2,  3,  4,  5,  6,  7,  8,  9  10, 11, 12*/
	static const int f1[] = { 3,  3,  3,  3,  3,  3,  3,  4,  5,  6,  7,  8,  9};
	static const int f2[] = { 0,  0,  0,  0,  0,  0,  0,  4,  8, 12, 16, 20, 24};

	if (UNEXPECTED(size <= 2)) return 0;
	n = zend_mm_small_size_to_bit(size - 1);
	return ((size-1) >> f1[n]) + f2[n];
#else
    unsigned int t1, t2;

    if (size <= 64) {
        /* we need to support size == 0 ... */
        return (size - !!size) >> 3;
    } else {
        t1 = size - 1;
        t2 = zend_mm_small_size_to_bit(t1) - 3;
        t1 = t1 >> t2;
        t2 = t2 - 3;
        t2 = t2 << 2;
        return (int)(t1 + t2);
    }
#endif
}

int main()
{
    printf("%d\n",zend_mm_small_size_to_bin(48));

    return 0;
}

```

按照上述环境传入48，发现结果变成了5，于是展开思考为什么会变成5，上面的代码显示，如果size小于64，和大于64都会进行一些处理，我们看一下zend_mm_heap的结构体

```
struct _zend_mm_heap {
#if ZEND_MM_CUSTOM
	int                use_custom_heap;
#endif
#if ZEND_MM_STORAGE
	zend_mm_storage   *storage;
#endif
#if ZEND_MM_STAT
	size_t             size;                    /* current memory usage */
	size_t             peak;                    /* peak memory usage */
#endif
	zend_mm_free_slot *free_slot[ZEND_MM_BINS]; /* free lists for small sizes */
#if ZEND_MM_STAT || ZEND_MM_LIMIT
	size_t             real_size;               /* current size of allocated pages */
#endif
#if ZEND_MM_STAT
	size_t             real_peak;               /* peak size of allocated pages */
#endif
#if ZEND_MM_LIMIT
	size_t             limit;                   /* memory limit */
	int                overflow;                /* memory overflow flag */
#endif

	zend_mm_huge_list *huge_list;               /* list of huge allocated blocks */

	zend_mm_chunk     *main_chunk;
	zend_mm_chunk     *cached_chunks;			/* list of unused chunks */
	int                chunks_count;			/* number of alocated chunks */
	int                peak_chunks_count;		/* peak number of allocated chunks for current request */
	int                cached_chunks_count;		/* number of cached chunks */
	double             avg_chunks_count;		/* average number of chunks allocated per request */
	int                last_chunks_delete_boundary; /* numer of chunks after last deletion */
	int                last_chunks_delete_count;    /* number of deletion over the last boundary */
#if ZEND_MM_CUSTOM
	union {
		struct {
			void      *(*_malloc)(size_t);
			void       (*_free)(void*);
			void      *(*_realloc)(void*, size_t);
		} std;
		struct {
			void      *(*_malloc)(size_t ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
			void       (*_free)(void*  ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
			void      *(*_realloc)(void*, size_t  ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC);
		} debug;
	} custom_heap;
#endif
};
```

free_list的栈是30，如果说没有被初始化，则调用，我们看一下具体实现

```
static zend_never_inline void *zend_mm_alloc_small_slow(zend_mm_heap *heap, uint32_t bin_num ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
{
    zend_mm_chunk *chunk;
    int page_num;
	zend_mm_bin *bin;
	zend_mm_free_slot *p, *end;

#if ZEND_DEBUG
	bin = (zend_mm_bin*)zend_mm_alloc_pages(heap, bin_pages[bin_num], bin_data_size[bin_num] ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#else
	bin = (zend_mm_bin*)zend_mm_alloc_pages(heap, bin_pages[bin_num] ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
#endif
	if (UNEXPECTED(bin == NULL)) {
		/* insufficient memory */
		return NULL;
	}

	chunk = (zend_mm_chunk*)ZEND_MM_ALIGNED_BASE(bin, ZEND_MM_CHUNK_SIZE);
	page_num = ZEND_MM_ALIGNED_OFFSET(bin, ZEND_MM_CHUNK_SIZE) / ZEND_MM_PAGE_SIZE;
	chunk->map[page_num] = ZEND_MM_SRUN(bin_num);
	if (bin_pages[bin_num] > 1) {
		uint32_t i = 1;

		do {
			chunk->map[page_num+i] = ZEND_MM_NRUN(bin_num, i);
			i++;
		} while (i < bin_pages[bin_num]);
	}

	/* create a linked list of elements from 1 to last */
	end = (zend_mm_free_slot*)((char*)bin + (bin_data_size[bin_num] * (bin_elements[bin_num] - 1)));
	heap->free_slot[bin_num] = p = (zend_mm_free_slot*)((char*)bin + bin_data_size[bin_num]);
	do {
		p->next_free_slot = (zend_mm_free_slot*)((char*)p + bin_data_size[bin_num]);
#if ZEND_DEBUG
		do {
			zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)p + bin_data_size[bin_num] - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
			dbg->size = 0;
		} while (0);
#endif
		p = (zend_mm_free_slot*)((char*)p + bin_data_size[bin_num]);
	} while (p != end);

	/* terminate list using NULL */
	p->next_free_slot = NULL;
#if ZEND_DEBUG
		do {
			zend_mm_debug_info *dbg = (zend_mm_debug_info*)((char*)p + bin_data_size[bin_num] - ZEND_MM_ALIGNED_SIZE(sizeof(zend_mm_debug_info)));
			dbg->size = 0;
		} while (0);
#endif

	/* return first element */
	return (char*)bin;
}
```

在这里面我们看到了第一个十分关键的函数

```
bin = (zend_mm_bin*)zend_mm_alloc_pages(heap, bin_pages[bin_num] ZEND_FILE_LINE_RELAY_CC ZEND_FILE_LINE_ORIG_RELAY_CC);
```

我看到了一个十分长的函数

```
static void *zend_mm_alloc_pages(zend_mm_heap *heap, uint32_t pages_count ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
#endif
{
	zend_mm_chunk *chunk = heap->main_chunk;
	uint32_t page_num, len;
	int steps = 0;

	while (1) {
		if (UNEXPECTED(chunk->free_pages < pages_count)) {
			goto not_found;
#if 0
		} else if (UNEXPECTED(chunk->free_pages + chunk->free_tail == ZEND_MM_PAGES)) {
			if (UNEXPECTED(ZEND_MM_PAGES - chunk->free_tail < pages_count)) {
				goto not_found;
			} else {
				page_num = chunk->free_tail;
				goto found;
			}
		} else if (0) {
			/* First-Fit Search */
			int free_tail = chunk->free_tail;
			zend_mm_bitset *bitset = chunk->free_map;
			zend_mm_bitset tmp = *(bitset++);
			int i = 0;

			while (1) {
				/* skip allocated blocks */
				while (tmp == (zend_mm_bitset)-1) {
					i += ZEND_MM_BITSET_LEN;
					if (i == ZEND_MM_PAGES) {
						goto not_found;
					}
					tmp = *(bitset++);
				}
				/* find first 0 bit */
				page_num = i + zend_mm_bitset_nts(tmp);
				/* reset bits from 0 to "bit" */
				tmp &= tmp + 1;
				/* skip free blocks */
				while (tmp == 0) {
					i += ZEND_MM_BITSET_LEN;
					len = i - page_num;
					if (len >= pages_count) {
						goto found;
					} else if (i >= free_tail) {
						goto not_found;
					}
					tmp = *(bitset++);
				}
				/* find first 1 bit */
				len = (i + zend_ulong_ntz(tmp)) - page_num;
				if (len >= pages_count) {
					goto found;
				}
				/* set bits from 0 to "bit" */
				tmp |= tmp - 1;
			}
#endif
		} else {
			/* Best-Fit Search */
			int best = -1;
			uint32_t best_len = ZEND_MM_PAGES;
			uint32_t free_tail = chunk->free_tail;
			zend_mm_bitset *bitset = chunk->free_map;
			zend_mm_bitset tmp = *(bitset++);
			uint32_t i = 0;

			while (1) {
				/* skip allocated blocks */
				while (tmp == (zend_mm_bitset)-1) {
					i += ZEND_MM_BITSET_LEN;
					if (i == ZEND_MM_PAGES) {
						if (best > 0) {
							page_num = best;
							goto found;
						} else {
							goto not_found;
						}
					}
					tmp = *(bitset++);
				}
				/* find first 0 bit */
				page_num = i + zend_mm_bitset_nts(tmp);
				/* reset bits from 0 to "bit" */
				tmp &= tmp + 1;
				/* skip free blocks */
				while (tmp == 0) {
					i += ZEND_MM_BITSET_LEN;
					if (i >= free_tail || i == ZEND_MM_PAGES) {
						len = ZEND_MM_PAGES - page_num;
						if (len >= pages_count && len < best_len) {
							chunk->free_tail = page_num + pages_count;
							goto found;
						} else {
							/* set accurate value */
							chunk->free_tail = page_num;
							if (best > 0) {
								page_num = best;
								goto found;
							} else {
								goto not_found;
							}
						}
					}
					tmp = *(bitset++);
				}
				/* find first 1 bit */
				len = i + zend_ulong_ntz(tmp) - page_num;
				if (len >= pages_count) {
					if (len == pages_count) {
						goto found;
					} else if (len < best_len) {
						best_len = len;
						best = page_num;
					}
				}
				/* set bits from 0 to "bit" */
				tmp |= tmp - 1;
			}
		}

not_found:
		if (chunk->next == heap->main_chunk) {
get_chunk:
			if (heap->cached_chunks) {
				heap->cached_chunks_count--;
				chunk = heap->cached_chunks;
				heap->cached_chunks = chunk->next;
			} else {
#if ZEND_MM_LIMIT
				if (UNEXPECTED(heap->real_size + ZEND_MM_CHUNK_SIZE > heap->limit)) {
					if (zend_mm_gc(heap)) {
						goto get_chunk;
					} else if (heap->overflow == 0) {
#if ZEND_DEBUG
						zend_mm_safe_error(heap, "Allowed memory size of %zu bytes exhausted at %s:%d (tried to allocate %zu bytes)", heap->limit, __zend_filename, __zend_lineno, size);
#else
						zend_mm_safe_error(heap, "Allowed memory size of %zu bytes exhausted (tried to allocate %zu bytes)", heap->limit, ZEND_MM_PAGE_SIZE * pages_count);
#endif
						return NULL;
					}
				}
#endif
				chunk = (zend_mm_chunk*)zend_mm_chunk_alloc(heap, ZEND_MM_CHUNK_SIZE, ZEND_MM_CHUNK_SIZE);
				if (UNEXPECTED(chunk == NULL)) {
					/* insufficient memory */
					if (zend_mm_gc(heap) &&
					    (chunk = (zend_mm_chunk*)zend_mm_chunk_alloc(heap, ZEND_MM_CHUNK_SIZE, ZEND_MM_CHUNK_SIZE)) != NULL) {
						/* pass */
					} else {
#if !ZEND_MM_LIMIT
						zend_mm_safe_error(heap, "Out of memory");
#elif ZEND_DEBUG
						zend_mm_safe_error(heap, "Out of memory (allocated %zu) at %s:%d (tried to allocate %zu bytes)", heap->real_size, __zend_filename, __zend_lineno, size);
#else
						zend_mm_safe_error(heap, "Out of memory (allocated %zu) (tried to allocate %zu bytes)", heap->real_size, ZEND_MM_PAGE_SIZE * pages_count);
#endif
						return NULL;
					}
				}
#if ZEND_MM_STAT
				do {
					size_t size = heap->real_size + ZEND_MM_CHUNK_SIZE;
					size_t peak = MAX(heap->real_peak, size);
					heap->real_size = size;
					heap->real_peak = peak;
				} while (0);
#elif ZEND_MM_LIMIT
				heap->real_size += ZEND_MM_CHUNK_SIZE;

#endif
			}
			heap->chunks_count++;
			if (heap->chunks_count > heap->peak_chunks_count) {
				heap->peak_chunks_count = heap->chunks_count;
			}
			zend_mm_chunk_init(heap, chunk);
			page_num = ZEND_MM_FIRST_PAGE;
			len = ZEND_MM_PAGES - ZEND_MM_FIRST_PAGE;
			goto found;
		} else {
			chunk = chunk->next;
			steps++;
		}
	}

found:
	if (steps > 2 && pages_count < 8) {
		/* move chunk into the head of the linked-list */
		chunk->prev->next = chunk->next;
		chunk->next->prev = chunk->prev;
		chunk->next = heap->main_chunk->next;
		chunk->prev = heap->main_chunk;
		chunk->prev->next = chunk;
		chunk->next->prev = chunk;
	}
	/* mark run as allocated */
	chunk->free_pages -= pages_count;
	zend_mm_bitset_set_range(chunk->free_map, page_num, pages_count);
	chunk->map[page_num] = ZEND_MM_LRUN(pages_count);
	if (page_num == chunk->free_tail) {
		chunk->free_tail = page_num + pages_count;
	}
	return ZEND_MM_PAGE_ADDR(chunk, page_num);
}
```

我们先看

```
zend_mm_chunk *chunk = heap->main_chunk;
```

他的结构体定义是

```
zend_mm_chunk     *main_chunk;
```

通过gdb看到了结果

```
(gdb) print *heap->main_chunk
$14 = {heap = 0x7fffeea00040, next = 0x7fffeea00000, prev = 0x7fffeea00000, 
  free_pages = 511, free_tail = 1, num = 0, 
  reserve = '\000' <repeats 27 times>, heap_slot = {use_custom_heap = 0, 
    storage = 0x0, size = 48, peak = 48, free_slot = {0x0 <repeats 30 times>}, 
    real_size = 2097152, real_peak = 2097152, limit = 9223372036854775807, 
    overflow = 0, huge_list = 0x0, main_chunk = 0x7fffeea00000, 
    cached_chunks = 0x0, chunks_count = 1, peak_chunks_count = 1, 
    cached_chunks_count = 0, avg_chunks_count = 1, 
    last_chunks_delete_boundary = 0, last_chunks_delete_count = 0, 
    custom_heap = {std = {_malloc = 0x0, _free = 0x0, _realloc = 0x0}, 
      debug = {_malloc = 0x0, _free = 0x0, _realloc = 0x0}}}, free_map = {1, 
    0, 0, 0, 0, 0, 0, 0}, map = {1073741825, 0 <repeats 511 times>}}
(gdb) print pages_count
$15 = 1

```

也就是说 

```
if (UNEXPECTED(chunk->free_pages < pages_count)) {
			goto not_found;
		
```

这里是判断内存分页是否充足，如果不充足则跳入not found，我们继续看在空闲内存充足的时候，最好的分页长度是ZEND_MM_PAGES是512，这里我又看到了一个结构体zend_mm_bitset	

```
typedef zend_ulong zend_mm_bitset; 
```

他是uint32，在这里我看到了

```
zend_mm_bitset *bitset = chunk->free_map;
```

现在又要去看free_map了，free_map的定义为

```
struct _zend_mm_chunk {
	zend_mm_heap      *heap;
	zend_mm_chunk     *next;
	zend_mm_chunk     *prev;
	uint32_t           free_pages;				/* number of free pages */
	uint32_t           free_tail;               /* number of free pages at the end of chunk */
	uint32_t           num;
	char               reserve[64 - (sizeof(void*) * 3 + sizeof(uint32_t) * 3)];
	zend_mm_heap       heap_slot;               /* used only in main chunk */
	zend_mm_page_map   free_map;                /* 512 bits or 64 bytes */
	zend_mm_page_info  map[ZEND_MM_PAGES];      /* 2 KB = 512 * 4 */
};
```

这里的free_map类型是zend_mm_page_map，他的定义是
```
#define ZEND_MM_BITSET_LEN		(sizeof(zend_mm_bitset) * 8)       /* 32 or 64 */
#define ZEND_MM_PAGE_MAP_LEN	(ZEND_MM_PAGES / ZEND_MM_BITSET_LEN) /* 16 or 8 */

typedef zend_mm_bitset zend_mm_page_map[ZEND_MM_PAGE_MAP_LEN];     /* 64B */
```

这是一个长度是8或者16的uint32的数组，我们可以用gdb查看当前的内存状况

```
{1, 0, 0, 0, 0, 0, 0, 0}
```

我使用gdb查看了运行结果，在这里我就非常疑惑了

```
(gdb) print chunk->free_map
$1 = {1, 0, 0, 0, 0, 0, 0, 0}
(gdb) print *bigset
No symbol "bigset" in current context.
(gdb) print *bitset
$2 = 0
(gdb) print bitset
$3 = (zend_mm_bitset *) 0x7ffff3e001c8
(gdb) print *bitset
$4 = 0
(gdb) print tmp
$5 = 1
(gdb) print tmp
$6 = 1
(gdb) print *tmp
Cannot access memory at address 0x1
(gdb) print tmp
$7 = 1
(gdb) print tmp
$8 = 1
(gdb) print tmp
$9 = 1
(gdb) print chunk->free_map
$10 = {1, 0, 0, 0, 0, 0, 0, 0}
(gdb) 

```

因为我疑惑的地方是tmp是1，他为什么不是0呢？
