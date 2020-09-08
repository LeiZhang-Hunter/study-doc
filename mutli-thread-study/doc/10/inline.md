
inline 函数的方方面面。由于inline函数的关系，c++源码里调用一个函数并不意味着生成目标代码里也会做一次真正的函数调用，现在的编译器已经可以自动判断一个函数是否适合inline，因此inline关键字在源文件中往往不是必须的。当然在头文件里inline 是有必要的，为了防止编译器抱怨重复定义。现在的c++编译器采用重复代码取消的方法来避免 重复定义。也就是说编译器无法inline展开的话，每个编译单元都会生成inline的目标代码，然后编译器都会生成inline函数的目标代码，然后编译器会从多份实现中任选一份保留，其余的就丢弃。如果编译器能够展开inline函数，那就不必为之单独生成目标代码了。

如何判断一个c++可执行文件是debug build 还是release build？换言之，如何判断一个文件是-O0 还是 -O2 编译？我通常的做法是看class template的短成员函数有没有被inline展开。

例如


	
	#include <vector>
	#include <stdio.h>
	
	
	int main() {
	    std::vector<int> vi;
	    printf("%zd\n", vi.size());
	}

g++ -Wall main.cpp

查看 inline 情况

	zhanglei@zhanglei-virtual-machine:~/ourc/test$ nm ./a.out |grep size|c++filt
	0000000000001384 W std::vector<int, std::allocator<int> >::size() const

我们发现size 没有被inline展开 

如果我们使用-O2

	g++ -Wall -O2 main.cpp

看运行结果

	zhanglei@zhanglei-virtual-machine:~/ourc/test$ nm ./a.out |grep size|c++filt
	zhanglei@zhanglei-virtual-machine:~/ourc/test$ 

我们发现找不到 这个函数符号了，原因就是他已经被编译器inline展开了

注意编译器为我们自动生成的析构函数也是inline函数，有时候我们要故意out-line,防止代码膨胀或者出现编译错误

现在编译器有了link time code generation，编译器已经不需要看到inline了，inline留给连接器去做
