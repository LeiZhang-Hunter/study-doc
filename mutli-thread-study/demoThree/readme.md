# 内容介绍

这篇笔记学习自陈硕先生的 c++ 多线程服务器编程

陈硕先生在书籍开篇用这个例子说了，c++多线程服务器编程中，对对象生命周期的管理是一个十分重要的事情

注意一个点 就是 智能指针被销毁后注意内存泄露，同步删除调map中的值，在这里要重点说一下bind和placeholder我是第一次接触到这个语法

在10.4中c++ primer 对这个语法进行了介绍

标准库bind函数

我们可以解决向check_size传递一个参数长度的问题，方法是使用一个新的名字为bind的标准函数库，他定义在functional中。可以把bind函数看做一个通用的函数适配器，他接收一个可调用对象，生成一个新的可调用对象来适应原来的参数列表。

调用bind的一般形式是

auto newCallable = bind(callable,arg_list);
arglist可以使用占位符

```
using namespace std::placeholders;
```

当然你使用bind是可以重排参数的位置的

```
sort(words.begin(),words.end(),isShort);
sort(words.begin(),words.end(),bind(isShort,_2,_1));
```

bind传参的时候有的时候我们希望用引用的形式，那么我们必须使用ref

一个简单的demo

```
#include <memory>
#include <iostream>
#include <string.h>
#include <vector>
#include <pthread.h>
#include <map>
#include <functional>
using namespace std::placeholders;
int sum(int a,int b)
{
    return  a-b;
}

int main()
{
    int c = 1;
    int d = 2;
    std::function<int(int,int)> p = std::bind(sum,_2,_1);
    std::cout<<p(1,2)<<std::endl;
    return  0;

}
```

###总结一下上个demo

虽然看似正确，确是暗藏风险，因为StockFactory::get() 把原始指针 this 保存到了 boost::function，如 果 StockFactory 的 生 命 期 比 Stock 短, 那 么 Stock 析 构 时 去 回 调 StockFactory::deleteStock 就会 core dump。因为你无法知道这个指针是否存活，所以我们可以enable_shared_from_this