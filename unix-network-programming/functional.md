我们可以解决向check_size传递一个参数长度的问题，方法是使用一个新的名字为bind的标准函数库，他定义在functional中。可以把bind函数看做一个通用的函数适配器，他接收一个可调用对象，生成一个新的可调用对象来适应原来的参数列表。

调用bind的一般形式是

```
auto newCallable = bind(callable,arg_list);
```

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




