#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <functional>
#include <iostream>
using namespace std;
class Foo{
public:
    void methodA()
    {
        printf("222\n");
    }
    void methodInt(int a)
    {
        printf("%d\n",a);
    }
    void methodString(const string& str)
    {
        cout<<str<<endl;
    }
};

class Bar{
public:
    void methodB();
};

int main()
{
    function<void ()>f1;
    Foo foo;
    f1 = std::bind(&Foo::methodA,&foo);
    f1();

    // 注意,bind 拷贝的是实参类型 (const char*),不是形参类型 (string)
// 这里形参中的 string 对象的构造发生在调用 f1 的时候,而非 bind 的时候,
// 因此要留意 bind 的实参 (cosnt char*) 的生命期,它应该不短于 f1 的生命期。
// 必要时可通过 bind(&Foo::methodString, &foo, string(aTempBuf)) 来保证安全
    function<void (int)>f2;
    f2 = std::bind(&Foo::methodInt,&foo,placeholders::_1);
    f2(33);

    f1 = std::bind(&Foo::methodString, &foo, "hello");
    f1(); // 调用 foo.methodString("hello")
    return 0;
}