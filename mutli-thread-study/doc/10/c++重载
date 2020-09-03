写一个简单的demo 看一下重载

foo.cpp

	//
	// Created by zhanglei on 2020/9/4.
	//
	
	int foo(int x)
	{
	    return 100;
	}
	
	int foo(bool x)
	{
	    return true;
	}

生产.o 文件，目标文件

	g++ -c foo.cpp

.o文件就是对象文件,是可重定向文件的一种,通常以ELF格式保存，里面包含了对各个函数的入口标记，描述，当程序要执行时还需要链接(link).链接就是把多个.o文件链成一个可执行文件。

我们用nm去看记录的函数名字

	zhanglei@zhanglei-virtual-machine:~/ourc/test$ nm foo.o 
	0000000000000012 T _Z3foob
	0000000000000000 T _Z3fooi

再具体还原函数名字 可以用 c++filt

	zhanglei@zhanglei-virtual-machine:~/ourc/test$ c++filt _Z3foob _Z3fooi
	foo(bool)
	foo(int)

我们发现返回类型并不参与重载，如果一个源文件用到了重载函数，但是它看到的函数原型声明的返回类型是错误的，连接器无法察觉



	#include <stdio.h>
	
	void foo(bool);
	
	int foo(int x)
	{
	    return 100;
	}
	
	
	int main() {
	
	    foo(true);
	
	}

比如上面这个例子 如果我们这么写编译器找不到错误，这将会在运行时出现问题，如果返回的是class
