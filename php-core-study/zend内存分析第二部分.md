我需要写一个demo来完成我的学习

```
#include <stdio.h>
#include <stdlib.h>
long arr[8] = {1, 0, 0, 0, 0, 0, 0, 0};
int main()
{
    long* bitset = arr;
    long tmp = *(bitset++);
    printf("%ld\n",tmp);
    exit(0);
}

```

这里我就开始思考为什么tmp是0注意不是1啊，然后我打印出了地址

```
0x55e59cd77020
0x55e59cd77028
1
```

这里看到指针可以看出结果这个运算    long tmp = *(bitset++);意味着是先返回结果后偏移，我不明白zend为什么要这么做，这样做真的十分鸡肋！！！

然后我们继续看这一段

```
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
```

如果说这个tmp溢出了。给i接上32个或者64个字节值，tmp = *(bitset++);这一步是十分明显的赋值tmp，然后继续偏移，一直偏移512页都偏移完了，如果说best大于0，则跳转入found，如果说小于0，由于上面已经定义过了，best 为-1所以直接接入not_found的代码块,这就说明内存池里面的所有内存页都被用完了

我们再看下面这一行

```
page_num = i + zend_mm_bitset_nts(tmp);
```

继续分析这个函数zend_mm_bitset_nts

```
/* number of trailing set (1) bits */
static zend_always_inline int zend_mm_bitset_nts(zend_mm_bitset bitset)
{
#if (defined(__GNUC__) || __has_builtin(__builtin_ctzl)) && SIZEOF_ZEND_LONG == SIZEOF_LONG && defined(PHP_HAVE_BUILTIN_CTZL)
	return __builtin_ctzl(~bitset);
#elif (defined(__GNUC__) || __has_builtin(__builtin_ctzll)) && defined(PHP_HAVE_BUILTIN_CTZLL)
	return __builtin_ctzll(~bitset);
#elif defined(_WIN32)
	unsigned long index;

#if defined(_WIN64)
	if (!BitScanForward64(&index, ~bitset)) {
#else
	if (!BitScanForward(&index, ~bitset)) {
#endif
		/* undefined behavior */
		return 32;
	}

	return (int)index;
#else
	int n;

	if (bitset == (zend_mm_bitset)-1) return ZEND_MM_BITSET_LEN;

	n = 0;
#if SIZEOF_ZEND_LONG == 8
	if (sizeof(zend_mm_bitset) == 8) {
		if ((bitset & 0xffffffff) == 0xffffffff) {n += 32; bitset = bitset >> Z_UL(32);}
	}
#endif
	if ((bitset & 0x0000ffff) == 0x0000ffff) {n += 16; bitset = bitset >> 16;}
	if ((bitset & 0x000000ff) == 0x000000ff) {n +=  8; bitset = bitset >>  8;}
	if ((bitset & 0x0000000f) == 0x0000000f) {n +=  4; bitset = bitset >>  4;}
	if ((bitset & 0x00000003) == 0x00000003) {n +=  2; bitset = bitset >>  2;}
	return n + (bitset & 1);
#endif
}
```

这段代码真是让我看的头大，注释上说是获取第一个0字节的位置，因为注释是number of trailing set (1) bits，但是我通过

```
	if ((bitset & 0x0000ffff) == 0x0000ffff) {n += 16; bitset = bitset >> 16;}
	if ((bitset & 0x000000ff) == 0x000000ff) {n +=  8; bitset = bitset >>  8;}
	if ((bitset & 0x0000000f) == 0x0000000f) {n +=  4; bitset = bitset >>  4;}
	if ((bitset & 0x00000003) == 0x00000003) {n +=  2; bitset = bitset >>  2;}
	return n + (bitset & 1);
```

认为这是在计算第一个0出现的位置，满足一定条件就会向右移动，在这里我猜测他是找寻一个块内可用的首地址

```
tmp &= tmp + 1;
```
这个是做什么的也不是很明白

```
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
```

ZEND_MM_BITSET_LEN是32或者64，如果说页数大于尾部的页，或者页数大于512个总页，用总页数减去page_num，这里得到的好像是剩余页数

继续查看
```
static const uint32_t bin_pages[] = {
  ZEND_MM_BINS_INFO(_BIN_DATA_PAGES, x, y)
};
```

再继续看

```
/* num, size, count, pages */
#define ZEND_MM_BINS_INFO(_, x, y) \
	_( 0,    8,  512, 1, x, y) \
	_( 1,   16,  256, 1, x, y) \
	_( 2,   24,  170, 1, x, y) \
	_( 3,   32,  128, 1, x, y) \
	_( 4,   40,  102, 1, x, y) \
	_( 5,   48,   85, 1, x, y) \
	_( 6,   56,   73, 1, x, y) \
	_( 7,   64,   64, 1, x, y) \
	_( 8,   80,   51, 1, x, y) \
	_( 9,   96,   42, 1, x, y) \
	_(10,  112,   36, 1, x, y) \
	_(11,  128,   32, 1, x, y) \
	_(12,  160,   25, 1, x, y) \
	_(13,  192,   21, 1, x, y) \
	_(14,  224,   18, 1, x, y) \
	_(15,  256,   16, 1, x, y) \
	_(16,  320,   64, 5, x, y) \
	_(17,  384,   32, 3, x, y) \
	_(18,  448,    9, 1, x, y) \
	_(19,  512,    8, 1, x, y) \
	_(20,  640,   32, 5, x, y) \
	_(21,  768,   16, 3, x, y) \
	_(22,  896,    9, 2, x, y) \
	_(23, 1024,    8, 2, x, y) \
	_(24, 1280,   16, 5, x, y) \
	_(25, 1536,    8, 3, x, y) \
	_(26, 1792,   16, 7, x, y) \
	_(27, 2048,    8, 4, x, y) \
	_(28, 2560,    8, 5, x, y) \
	_(29, 3072,    4, 3, x, y)
```

我们再看一下这个宏的定义

```
#define ZEND_MM_ALIGNED_BASE(size, alignment) \
	(((size_t)(size)) & ~((alignment) - 1))
```