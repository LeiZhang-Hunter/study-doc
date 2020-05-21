#c++多线程系统编程精要

学习多线程系统编程要面临两个思维转变：

1.当前线程可能会被随时切换出去

2.多线程中事件发生顺序不会再有全局的先后关系

当线程被切换回来继续执行下一条语句的时候,全局数据可能已经被其他线程修改。例如在没有为指针p加锁的情况下，if(p && p->next){/**/}就有可能会
导致segfault,因为在逻辑与的前一个分支评估为true的那一刹那,p可能被其他线程设置为NULL或者被释放，后一个分支就访问了一个非法地址

在单cpu系统中，理论上我们可通过cpu执行的指令先后顺序来推演多线程的实际交织运行情况。在多核系统中，多个线程是并行执行的，我们甚至没有一个全局
时钟来为每个事件编号。在没有适当同步的情况下，多个cpu上运行的多个线程中的事件发生顺序是无法预测的，在引入了适当的同步之后，事件才会有先后

多线程的正确性不依赖于任何线程的执行速度，不能通过原地等待sleep来确定他的事件已经发生，而必须要通过适当的同步来让当前线程能看到其他线程的执
行结果。无论线程执行的快与慢，程序都应该能正常执行。

看到书中的这个例子我想到了我之前logSentry让线程终止其实也暗藏风险，好在没有跨现场使用这种函数

下面看一下这个demo和例子

```
bool running = false;

void threadFunc()
{
    while(running)
    {
        //get task from queue
    }
}

void start()
{
    muduo::thread t(threadFunc);
    t.start();
    running = true;
}
```

这段代码在系统负载高的时候，running会被推迟赋值，导致系统直接退出，正确做法是把running 放在pthread_create的前面

####4.1基本的线程原语

POSIX threads 的函数有110多个，真正常用的也就10几个

11个基本的函数分别是：

2个：线程的创建和等待结束。
4个：mutex的创建 销毁 加锁 解锁
5个：条件变量的创建 销毁 等待 通知 广播

用thread mutex 和 condition 可以轻松完成多线程任务。

一些函数可以酌情使用:

1.pthread_once 封装muduo::Singleton<T>。其实不如直接使用全局变量。

2.pthread_key* 封装为muduo::ThreadLocal<T>。可以考虑用__thread替换之。

读到这里我就思考pthread_key是什么？我记得这个在<<unix>>网络编程里有过记载，在这里在回忆一下数据pthread_key_create,再次回头看一下unix
网络编程26章26.5线程特定数据

我们在把一个不可重入带有静态变量的函数带入多线程当中是十分危险的，这个静态变量无法保存各个线程的值。

使用线程特定数据。这个办法并不简单，有点事不需要变动程序调用顺序只是需要更改函数中的代码即可。

使用线程特定数据是让现有函数变成线程安全函数的有效办法

不同系统要求支持有限个线程特定数据。posix要求这个限制不小于128个。

pthread_create_key为我们创建一个不再使用的线程特定数据的key

除了key之外还提供了一个析构函数指针。

![](pthread_spec_key.png)

我们首先看一下旧版程序当中的readline

![](readline.png)

说实话我没有找到这个函数在多线程编程中有什么问题，因为我认为并没有使用任何全局变量和任何静态变量，不会出现可重入问题，我们在看一下书中的demo
，然后自己手写一下看看书中的代码是有什么作用的,这段代码是我在我ubuntu电脑上摘抄的。
```
#include <vector>
#include <string>
#include <assert.h>
#include <iostream>
#include <zconf.h>
#include <fcntl.h>

static pthread_key_t r1_key;
static pthread_once_t r1_once = PTHREAD_ONCE_INIT;
#define MAXLINE 1024

typedef struct{
    int r1_cnt;
    char *r1_bufptr;
    char r1_buf[MAXLINE];
}Rline;

static void readline_destructor(void* ptr)
{
    free(ptr);
}

static void readline_once(void)
{
    pthread_key_create(&r1_key,readline_destructor);
}

static ssize_t my_read(Rline *tsd,int fd,char* ptr)
{
    if(tsd->r1_cnt <= 0)
    {
        again:
        if((tsd->r1_cnt = read(fd,tsd->r1_buf,MAXLINE)) < 0)
        {
            if(errno == EINTR)
            {
                goto again;
            }
            return (-1);
        }else if(tsd->r1_cnt == 0)
        {
            return 0;
        }
        tsd->r1_bufptr = tsd->r1_buf;
    }

    tsd->r1_cnt--;
    *ptr = *tsd->r1_bufptr++;
    return(1);
}

size_t readline(int fd,void *vptr,size_t maxlen)
{
    ssize_t n,rc;

    char c, *ptr;

    void *tsd;

    pthread_once(&r1_once,readline_once);

    if((tsd = pthread_getspecific(r1_key)) == nullptr)
    {
        tsd = calloc(1,sizeof(Rline));
        pthread_setspecific(r1_key,tsd);
    }

    ptr = (char*)vptr;

    for(n=1;n<maxlen;n++)
    {
        if((rc = my_read((Rline*)tsd, fd, &c)) == 1)
        {
            *ptr++ = c;

            if(c == '\n')
            {
                break;
            }
        }else if(rc == 0)
        {
            *ptr = 0;
            return (n-1);
        }else{
            return -1;
        }
    }

    *ptr = 0;
    return n;
}
int main()
{
    int fd = open("/home/zhanglei/ourc/test/demoParser.y",O_RDWR);
    if(fd <0)
    {
        return -1;
    }
    char buf[BUFSIZ];
    int res = readline(fd,buf,BUFSIZ);
    if(res <0)
    {
        return -1;
    }
    printf("%d\n",res);
    printf("%s\n",buf);
}
```

析构函数：

我们的析构函数仅仅释放造诣分配的内存区域

一次性函数：

我们的一次性函数将由pthread_once调用一次，他只是创建由readline使用的健

Rline结构含有因在图3-18中声明为static而导致前述问题的三个变量。调用readline的每个线程都由readline动态分配一个Rline的结构，然后由析构函数
释放。

my_read函数

本函数第一个参数现在是指向预先问本线程分配的Rline结构的一个指针。

分配线程特定的数据

我们首先调用pthread_once,使得本进程的第一个调用readlink的线程通过调用pthread_once创建线程的特定健值

获取特定的数据指针

pthread_getspecific返回指向特定与本线程的Rline结构指针。然而如果这次是本线程首次调用readline，其返回一个空指针。在这种情况下，我们分配一个
Rline结构的空间，并且有calloc将r1_cnt成员初始化为0.然后我们调用pthread_setspecific为本线程存储这个指针。下一次调用readline的时候，
pthread_getspecific将返回这个刚刚存储的指针。

总结
读到这里已经了解到基本用法了，pthread_once初始化线程特定的key，然后根据特定key获取线程特定数据，没有的话重新设置

我们在看下muduo中的代码，如何把线程特定数据应用到实践当中 

