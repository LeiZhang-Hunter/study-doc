#redis布隆过滤器

####什么是布隆过滤器

判断一个东西一定不存在或者是可能存在或者一定不存在.


####redis中的布隆过滤器

之前的布隆过滤器中，可以使用redis的位图来实现，redis4.0之后提供了插件来实现

####布隆过滤器的实现

产生契机：

如果集合用线性存储，查找的时间复杂度为O(N)。

如果用红黑树或者AVL树存储，时间复杂度为O(logn)

如果用hash表存储，并且用链接地址法与平衡BST解决hash冲突，时间复杂度也为O[log(n/m)],m为hash分数桶

数量变多了 内存空间会变大，而且查找效率也会变慢

####设计思想：

BF是一个长度为M比特的位数组与k个哈希函数组成的数据结构。位数组初始化为0，所有hash函数都可以分别把输入数据尽量的分散。

当插入下一个元素的时候，将其数组分别输入k个hash函数，产生k个哈希值。以hash值作为数组中的下标，将所有k个对应的比特位置为1。

当要查询一个元素的时候

比如说baidu  tengxun  
经过hash 函数 映射到对应的位上 比如说百度是 1 4 7  tengxun 是 2 4 8 这些位设置为 1

这时候去查找 panglu 返回的是 1 5 7 去查找 5 是不存在的映射为  0，则表明不存在

####具体实现 

```
Justin Sobel写的一个位操作的哈希函数。

public function JSHash($string,$len = null)
{
    $hash = 1315423911;
    $len || $len = strlen($string);

    for ($i=0; $i<$len; $i++) {
        $hash ^= (($hash << 5) + ord($string[$i]) + ($hash >> 2));
    }

    return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
}
```

其实看到这里我是比较关心他为什么选1315423911这个数字的,1315423911这个数字有什么寓意呢?我也不是很清楚为什么选这个

但是二进制是 

```
1001110011001111100011010100111
```

string 的每一个二维码 加上 hash左移5,加上hash 右移二

($hash % 0xFFFFFFFF)取余，然后再 & 0xFFFFFFFF 取位


直接贴一下这些hash函数的例子

参考地址:http://imhuchao.com/1271.html
```
<?php

class BloomFilterHash
{
    /**
     * 由Justin Sobel编写的按位散列函数
     */
    public function JSHash($string, $len = null)
    {
        $hash = 1315423911;
        $len || $len = strlen($string);
        for ($i = 0; $i < $len; $i++) {
            $hash ^= (($hash << 5) + ord($string[$i]) + ($hash >> 2));
        }
        return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
    }

    /**
     * 该哈希算法基于AT＆T贝尔实验室的Peter J. Weinberger的工作。
     * Aho Sethi和Ulman编写的“编译器（原理，技术和工具）”一书建议使用采用此特定算法中的散列方法的散列函数。
     */
    public function PJWHash($string, $len = null)
    {
        $bitsInUnsignedInt = 4 * 8; //（unsigned int）（sizeof（unsigned int）* 8）;
        $threeQuarters = ($bitsInUnsignedInt * 3) / 4;
        $oneEighth = $bitsInUnsignedInt / 8;
        $highBits = 0xFFFFFFFF << (int)($bitsInUnsignedInt - $oneEighth);
        $hash = 0;
        $test = 0;
        $len || $len = strlen($string);
        for ($i = 0; $i < $len; $i++) {
            $hash = ($hash << (int)($oneEighth)) + ord($string[$i]);
        }
        $test = $hash & $highBits;
        if ($test != 0) {
            $hash = (($hash ^ ($test >> (int)($threeQuarters))) & (~$highBits));
        }
        return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
    }

    /**
     * 类似于PJW Hash功能，但针对32位处理器进行了调整。它是基于UNIX的系统上的widley使用哈希函数。
     */
    public function ELFHash($string, $len = null)
    {
        $hash = 0;
        $len || $len = strlen($string);
        for ($i = 0; $i < $len; $i++) {
            $hash = ($hash << 4) + ord($string[$i]);
            $x = $hash & 0xF0000000;
            if ($x != 0) {
                $hash ^= ($x >> 24);
            }
            $hash &= ~$x;
        }
        return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
    }

    /**
     * 这个哈希函数来自Brian Kernighan和Dennis Ritchie的书“The C Programming Language”。
     * 它是一个简单的哈希函数，使用一组奇怪的可能种子，它们都构成了31 .... 31 ... 31等模式，它似乎与DJB哈希函数非常相似。
     */
    public function BKDRHash($string, $len = null)
    {
        $seed = 131;  # 31 131 1313 13131 131313 etc..
        $hash = 0;
        $len || $len = strlen($string);
        for ($i = 0; $i < $len; $i++) {
            $hash = (int)(($hash * $seed) + ord($string[$i]));
        }
        return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
    }

    /**
     * 这是在开源SDBM项目中使用的首选算法。
     * 哈希函数似乎对许多不同的数据集具有良好的总体分布。它似乎适用于数据集中元素的MSB存在高差异的情况。
     */
    public function SDBMHash($string, $len = null)
    {
        $hash = 0;
        $len || $len = strlen($string);
        for ($i = 0; $i < $len; $i++) {
            $hash = (int)(ord($string[$i]) + ($hash << 6) + ($hash << 16) - $hash);
        }
        return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
    }

    /**
     * 由Daniel J. Bernstein教授制作的算法，首先在usenet新闻组comp.lang.c上向世界展示。
     * 它是有史以来发布的最有效的哈希函数之一。
     */
    public function DJBHash($string, $len = null)
    {
        $hash = 5381;
        $len || $len = strlen($string);
        for ($i = 0; $i < $len; $i++) {
            $hash = (int)(($hash << 5) + $hash) + ord($string[$i]);
        }
        return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
    }

    /**
     * Donald E. Knuth在“计算机编程艺术第3卷”中提出的算法，主题是排序和搜索第6.4章。
     */
    public function DEKHash($string, $len = null)
    {
        $len || $len = strlen($string);
        $hash = $len;
        for ($i = 0; $i < $len; $i++) {
            $hash = (($hash << 5) ^ ($hash >> 27)) ^ ord($string[$i]);
        }
        return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
    }

    /**
     * 参考 http://www.isthe.com/chongo/tech/comp/fnv/
     */
    public function FNVHash($string, $len = null)
    {
        $prime = 16777619; //32位的prime 2^24 + 2^8 + 0x93 = 16777619
        $hash = 2166136261; //32位的offset
        $len || $len = strlen($string);
        for ($i = 0; $i < $len; $i++) {
            $hash = (int)($hash * $prime) % 0xFFFFFFFF;
            $hash ^= ord($string[$i]);
        }
        return ($hash % 0xFFFFFFFF) & 0xFFFFFFFF;
    }
}
```

具体使用

```
/**
 * 使用redis实现的布隆过滤器
 */
abstract class BloomFilterRedis
{
	/**
	 * 需要使用一个方法来定义bucket的名字
	 */
	protected $bucket;

	protected $hashFunction;

	public function __construct($config, $id)
	{
		if (!$this->bucket || !$this->hashFunction) {
			throw new Exception("需要定义bucket和hashFunction", 1);
		}
		$this->Hash = new BloomFilterHash;
		$this->Redis = new YourRedis; //假设这里你已经连接好了
	}

	/**
	 * 添加到集合中
	 */
	public function add($string)
	{
		$pipe = $this->Redis->multi();
		foreach ($this->hashFunction as $function) {
			$hash = $this->Hash->$function($string);
			$pipe->setBit($this->bucket, $hash, 1);
		}
		return $pipe->exec();
	}

	/**
	 * 查询是否存在, 存在的一定会存在, 不存在有一定几率会误判
	 */
	public function exists($string)
	{
		$pipe = $this->Redis->multi();
		$len = strlen($string);
		foreach ($this->hashFunction as $function) {
			$hash = $this->Hash->$function($string, $len);
			$pipe = $pipe->getBit($this->bucket, $hash);
		}
		$res = $pipe->exec();
		foreach ($res as $bit) {
			if ($bit == 0) {
				return false;
			}
		}
		return true;
	}

}
```

添加就是把bitmap中置顶的位数设置为1 检查是否存在就是 看一下对应的为是否为0 如果有1个是0，那么就是不存在的

####布隆过滤器的应用场景

可以应对数据量比较大的判断key是否存在的业务场景,相比于hashmap速度更快，但是不推荐保存经常变更的内容为key

但是存在填充率问题如果被填充的1超过一定比率则这种算法就失去了意义