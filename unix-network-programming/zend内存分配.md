我们继续来分析zend的内存分配，我们看到

```

zend_mm_alloc_heap(zend_mm_heap *heap, size_t size ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC)
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
		                                                                                                                                                                          v`dbg = zend_mm_get_debug_info(heap, ptr);
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