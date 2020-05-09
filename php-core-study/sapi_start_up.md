我们继续往下读,这时候看到了一个函数

	php_cgi_globals_ctor(&php_cgi_globals);
	
从名字上就可以看出这是一个初始化php_cgi_globals这个结构体的,这个结构体是

	typedef struct _php_cgi_globals_struct {
		HashTable user_config_cache;
		char *redirect_status_env;
		zend_bool rfc2616_headers;
		zend_bool nph;
		zend_bool check_shebang_line;
		zend_bool fix_pathinfo;
		zend_bool force_redirect;
		zend_bool discard_path;
		zend_bool fcgi_logging;
	#ifdef PHP_WIN32
		zend_bool impersonate;
	#endif
	} php_cgi_globals_struct;
	
我们看一下这个函数做了什么

	static void php_cgi_globals_ctor(php_cgi_globals_struct *php_cgi_globals)
	{
	#ifdef ZTS
		ZEND_TSRMLS_CACHE_UPDATE();
	#endif
		php_cgi_globals->rfc2616_headers = 0;
		php_cgi_globals->nph = 0;
		php_cgi_globals->check_shebang_line = 1;
		php_cgi_globals->force_redirect = 1;
		php_cgi_globals->redirect_status_env = NULL;
		php_cgi_globals->fix_pathinfo = 1;
		php_cgi_globals->discard_path = 0;
		php_cgi_globals->fcgi_logging = 1;
	#ifdef PHP_WIN32
		php_cgi_globals->impersonate = 0;
	#endif
		zend_hash_init(&php_cgi_globals->user_config_cache, 8, NULL, user_config_cache_entry_dtor, 1);
	}
		
这里我们要重点看一下

	zend_hash_init(&php_cgi_globals->user_config_cache, 8, NULL, user_config_cache_entry_dtor, 1);

心中打了一个问号，这个函数是做什么的？

我首先开始思考HashTable这个结构体，这是php数组一个zend底层结构我们现在看一下这个结构是什么样子的,在zend_types.h中,我看到了这个结构体

	struct _zend_array {
		zend_refcounted_h gc;
		union {
			struct {
				ZEND_ENDIAN_LOHI_4(
					zend_uchar    flags,
					zend_uchar    _unused,
					zend_uchar    nIteratorsCount,
					zend_uchar    _unused2)
			} v;
			uint32_t flags;
		} u;
		uint32_t          nTableMask;
		Bucket           *arData;
		uint32_t          nNumUsed;
		uint32_t          nNumOfElements;
		uint32_t          nTableSize;
		uint32_t          nInternalPointer;
		zend_long         nNextFreeElement;
		dtor_func_t       pDestructor;
	};
	
我们再看一下 zend_hash_init的函数定义

	#define zend_hash_init(ht, nSize, pHashFunction, pDestructor, persistent) \
		_zend_hash_init((ht), (nSize), (pDestructor), (persistent))

这个函数在内核中被定义为了_zend_hash_init,我们再进一步看_zend_hash_init这个函数是如何实现的

	ZEND_API void ZEND_FASTCALL _zend_hash_init(HashTable *ht, uint32_t nSize, dtor_func_t pDestructor, zend_bool persistent)
	{
		_zend_hash_init_int(ht, nSize, pDestructor, persistent);
	}
	
这个函数进一步调用了_zend_hash_init_int，那么我们在进去_zend_hash_init_int这个函数到底做了什么

	static zend_always_inline void _zend_hash_init_int(HashTable *ht, uint32_t nSize, dtor_func_t pDestructor, zend_bool persistent)
	{
		GC_SET_REFCOUNT(ht, 1);
		GC_TYPE_INFO(ht) = IS_ARRAY | (persistent ? (GC_PERSISTENT << GC_FLAGS_SHIFT) : (GC_COLLECTABLE << GC_FLAGS_SHIFT));
		HT_FLAGS(ht) = HASH_FLAG_STATIC_KEYS;
		ht->nTableMask = HT_MIN_MASK;
		HT_SET_DATA_ADDR(ht, &uninitialized_bucket);
		ht->nNumUsed = 0;
		ht->nNumOfElements = 0;
		ht->nInternalPointer = 0;
		ht->nNextFreeElement = 0;
		ht->pDestructor = pDestructor;
		ht->nTableSize = zend_hash_check_size(nSize);
	}
	
第一个宏 GC_SET_REFCOUNT

我们查看源码是这样的

	#define GC_SET_REFCOUNT(p, rc)		zend_gc_set_refcount(&(p)->gc, rc)

看到了这里我开始思考gc的类型是什么，看到这里是zend_refcounted_h，那么这个又是什么呢？

	typedef struct _zend_refcounted_h {
		uint32_t         refcount;			/* reference counter 32-bit */
		union {
			uint32_t type_info;
		} u;
	} zend_refcounted_h;
	
这里我就看不懂了，这是做什么为什么结构体赋值为1，下面还是老办法 我自己写一个demo

	#include <stdio.h>
	#include <sys/acct.h>


	typedef struct _zend_refcounted_h {
	    uint32_t         refcount;			/* reference counter 32-bit */
	    union {
		uint32_t type_info;
	    } u;
	} zend_refcounted_h;

	struct _test{
	    zend_refcounted_h gc;
	}test;

	#define GC_SET_REFCOUNT(p, rc)		zend_gc_set_refcount(&(p)->gc, rc)
	int main()
	{
	    printf("%p\n",&(&test)->gc);
	}
	
现在就清楚了&(p)->gc是获取zend_refcounted_h这个结构体地址的

下面我们继续看 zend_gc_set_refcount这个函数做了什么

	static zend_always_inline uint32_t zend_gc_set_refcount(zend_refcounted_h *p, uint32_t rc) {
		p->refcount = rc;
		return p->refcount;
	}
	
现在就清楚了，GC_SET_REFCOUNT这个宏的作用就是把zend_array 中的 gc 结构体中的refcount设置为1，这个地方我说实话还不是很不明白refcount这个变量是做什么的在这里我不会过早下结论，我们在继续看

	GC_TYPE_INFO(ht) = IS_ARRAY | (persistent ? (GC_PERSISTENT << GC_FLAGS_SHIFT) : (GC_COLLECTABLE << GC_FLAGS_SHIFT));
	
这个是做什么的 我们还是从第一个宏开始看

	#define GC_TYPE_INFO(p)				(p)->gc.u.type_info
	
IS_ARRAY这个我写过扩展是指导的这是用来判断是否是一个数组类型的，从这里我们就可以很容易下结论了,在zend_refcounted_h这个联合体中的type_info他存储了_zend_array(Hashtable)的类型,但是后面这一段程序 又一次 引起了我的疑问 这是做什么的呢？

	(persistent ? (GC_PERSISTENT << GC_FLAGS_SHIFT) : (GC_COLLECTABLE << GC_FLAGS_SHIFT))
	
我们先来分别看一下这几个宏的定义

	#define GC_PERSISTENT               (1<<7) /* allocated using malloc */ // 1000000(二进制)
	#define GC_FLAGS_SHIFT				0
	#define GC_COLLECTABLE				(1<<4) //1000(二进制 16)
	
	
如果说persistent这个持久标志位是1,那么就是GC_PERSISTENT，如果不是1就是GC_COLLECTABLE，根据注释可能感觉是申请内存是堆上还是栈上申请是否持久化这个问题在这里也只是估计不能下定论


又看到了这一行

	HT_FLAGS(ht) = HASH_FLAG_STATIC_KEYS;

我又看到了一个宏 HT_FLAGS,这个宏的定义

	#define HT_FLAGS(ht) (ht)->u.flags
	#define HASH_FLAG_STATIC_KEYS      (1<<4) /* long and interned strings */
	
这里初步推测是php 关联数组和索引数组类型,总之就是给这个hashtable 初始化了为16

接下来我又看到了这一行

	ht->nTableMask = HT_MIN_MASK;
	
看一下HT_MIN_MASK这个宏

	#define HT_MIN_MASK ((uint32_t) -2)
	
这个宏的答案是-2

又看到了比较重要的一行

	HT_SET_DATA_ADDR(ht, &uninitialized_bucket);
	
开始思考几个点.HT_SET_DATA_ADDR这个宏定义是什么样子的，uninitialized_bucket这个又是什么，根据名字好像叫未初始化的槽，我们首先看HT_SET_DATA_ADDR这个宏定义是什么样子的

	#define HT_SET_DATA_ADDR(ht, ptr) do { \
			(ht)->arData = (Bucket*)(((char*)(ptr)) + HT_HASH_SIZE((ht)->nTableMask)); \
		} while (0)
		
这里我们看到了一个结构体叫Bucket* 这个地址真的是闻名已久，我们在继续看一下这个宏是什么样子的

	typedef struct _Bucket {
		zval              val;
		zend_ulong        h;                /* hash value (or numeric index)   */
		zend_string      *key;              /* string key or NULL for numerics */
	} Bucket;
	
HT_HASH_SIZE这个宏是

	#define HT_HASH_SIZE(nTableMask) \
		(((size_t)(uint32_t)-(int32_t)(nTableMask)) * sizeof(uint32_t))
		
我们刚才已经看到了(ht)->nTableMask他的结果是-2,当然是在有符号的情况下，无符号的情况下是4294967294，HT_HASH_SIZE的结果变为了8，也就是说：

	(((char*)(ptr)) + HT_HASH_SIZE((ht)->nTableMask)); 
	
这个宏把ptr偏移了8个字节,我们再看这个ptr是什么

	static const uint32_t uninitialized_bucket[-HT_MIN_MASK] =
	{HT_INVALID_IDX, HT_INVALID_IDX};
	
uint32_t 两个正好占了8个字节 也就是说这是一个连续地址 arData前8个字节存储了uninitialized_bucket，而后面存储了一个Bucket，总结出 这是一个静态的分配方式类似于malloc 也可以固定住地址我们现在用一个demo写一下看一下这种分配方式

	#include <stdio.h>
	#include <stdint.h>

	static const uint32_t arr[2] = {-1,-2};

	typedef struct _test{
	    int a;
	    int b;
	    int c;
	}test;

	test* static_malloc(void* ptr)
	{
	    return (test*)((char*)ptr+8);
	}

	int main()
	{
	    test* allocData1 = static_malloc((void*)&arr);
	    test* allocData2 = static_malloc((void*)&arr);
	    test* allocData3 = static_malloc((void*)&arr);
	    printf("%p\n",&arr);
	    printf("%p\n",allocData1);
	    printf("%p\n",allocData2);
	    printf("%p\n",allocData3);
	}

运行结果是

```
0x55ce9b687788
0x55ce9b687790
0x55ce9b687790
0x55ce9b687790
```

剩下的这一段内容就十分容易理解了

	ht->nNumUsed = 0;
	ht->nNumOfElements = 0;
	ht->nInternalPointer = 0;
	ht->nNextFreeElement = 0;
	ht->pDestructor = pDestructor;
	
注意pDestructor这个应该是一个析构函数，我们再看一下这个函数的最后一个就可以了

	ht->nTableSize = zend_hash_check_size(nSize);
	
具体实现

	static zend_always_inline uint32_t zend_hash_check_size(uint32_t nSize)
	{
	#if defined(ZEND_WIN32)
		unsigned long index;
	#endif

		/* Use big enough power of 2 */
		/* size should be between HT_MIN_SIZE and HT_MAX_SIZE */
		if (nSize <= HT_MIN_SIZE) {
			return HT_MIN_SIZE;
		} else if (UNEXPECTED(nSize >= HT_MAX_SIZE)) {
			zend_error_noreturn(E_ERROR, "Possible integer overflow in memory allocation (%u * %zu + %zu)", nSize, sizeof(Bucket), sizeof(Bucket));
		}

	#if defined(ZEND_WIN32)
		if (BitScanReverse(&index, nSize - 1)) {
			return 0x2 << ((31 - index) ^ 0x1f);
		} else {
			/* nSize is ensured to be in the valid range, fall back to it
			   rather than using an undefined bis scan result. */
			return nSize;
		}
	#elif (defined(__GNUC__) || __has_builtin(__builtin_clz))  && defined(PHP_HAVE_BUILTIN_CLZ)
		return 0x2 << (__builtin_clz(nSize - 1) ^ 0x1f);
	#else
		nSize -= 1;
		nSize |= (nSize >> 1);
		nSize |= (nSize >> 2);
		nSize |= (nSize >> 4);
		nSize |= (nSize >> 8);
		nSize |= (nSize >> 16);
		return nSize + 1;
	#endif
	}
	
这里是检查hashtable尺寸的最小是8个也就是默认的HT_MIN_SIZE，最大的数字是2的31次方，根据情况判断还有0x04000000的

```
#define HT_MIN_SIZE 8

#if SIZEOF_SIZE_T == 4
# define HT_MAX_SIZE 0x04000000 /* small enough to avoid overflow checks */
# define HT_HASH_TO_BUCKET_EX(data, idx) \
	((Bucket*)((char*)(data) + (idx)))
# define HT_IDX_TO_HASH(idx) \
	((idx) * sizeof(Bucket))
# define HT_HASH_TO_IDX(idx) \
	((idx) / sizeof(Bucket))
#elif SIZEOF_SIZE_T == 8
# define HT_MAX_SIZE 0x80000000
# define HT_HASH_TO_BUCKET_EX(data, idx) \
	((data) + (idx))
# define HT_IDX_TO_HASH(idx) \
	(idx)
# define HT_HASH_TO_IDX(idx) \
	(idx)
#else
```
	
我开始使用gdb查看这个过程

```
gdb php-cgi
break _zend_hash_init_int
run
l 195,208
```

我们看一下这个运行结果

```
(gdb) print *ht
$2 = {gc = {refcount = 1, u = {type_info = 135}}, u = {v = {flags = 16 '\020', _unused = 0 '\000', nIteratorsCount = 0 '\000', _unused2 = 0 '\000'}, flags = 16}, nTableMask = 4294967294, 
  arData = 0x5555562286a0, nNumUsed = 0, nNumOfElements = 0, nTableSize = 0, nInternalPointer = 0, nNextFreeElement = 0, pDestructor = 0x555555ad0220 <user_config_cache_entry_dtor>}
(gdb) print ht->nTableMask
$3 = 4294967294
(gdb) print -ht->nTableMask
$4 = 2

```

我们重点要看一下arData

```
(gdb) print *ht->arData
$6 = {val = {value = {lval = 2336936577129475669, dval = 1.8178389318403981e-152, counted = 0x206e776f6e6b6e55, str = 0x206e776f6e6b6e55, arr = 0x206e776f6e6b6e55, obj = 0x206e776f6e6b6e55, 
      res = 0x206e776f6e6b6e55, ref = 0x206e776f6e6b6e55, ast = 0x206e776f6e6b6e55, zv = 0x206e776f6e6b6e55, ptr = 0x206e776f6e6b6e55, ce = 0x206e776f6e6b6e55, func = 0x206e776f6e6b6e55, ww = {
        w1 = 1852534357, w2 = 544110447}}, u1 = {v = {type = 108 'l', type_flags = 105 'i', u = {call_info = 29811, extra = 29811}}, type_info = 1953720684}, u2 = {next = 1953391904, 
      cache_slot = 1953391904, opline_num = 1953391904, lineno = 1953391904, num_args = 1953391904, fe_pos = 1953391904, fe_iter_idx = 1953391904, access_flags = 1953391904, property_guard = 1953391904, 
      constant_flags = 1953391904, extra = 1953391904}}, h = 2334395648803109234, key = 0x29642528}
```

现在我们看到了bucket的结构 一个zval 结构体 ，一个 zend_string

好到这里我们已经分析完了php_cgi_globals_ctor的最核心函数zend_hash_init 是不是非常简单啊！！！今天就分析到了这里