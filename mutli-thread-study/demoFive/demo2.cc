#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <functional>
#include <iostream>
using namespace std;


class Thread{
public:
    typedef std::function<void()> ThreadCallback;

    Thread(ThreadCallback cb) : cb_(cb)
    {

    }

    void start()
    {
        run();
    }

private:
    void run()
    {
        cb_();
    }

    ThreadCallback cb_;
};

class Foo
{
public:
    void runInThread()
    {
        cout<<"11"<<endl;
    }
    void runInAnotherThread(int)
    {
        cout<<"222"<<endl;
    }
};

int main()
{
    Foo foo;
    Thread thread1(std::bind(&Foo::runInThread, &foo));
    Thread thread2(std::bind(&Foo::runInAnotherThread, &foo, 43));
    thread1.start(); // 在两个线程中分别运行两个成员函数
    thread2.start();
}