# java核心卷14章的读书笔记

## 1.java里多线程是如何运行的

十分的基础和简单，直接继承java的Thread，然后写一个run函数，有点像c++的虚函数吧

这样运行的入口就在我们写的run上了，上一个十分简单的demo

```java
class Thread1 extends Thread{
    public void run()
    {
        while(!this.currentThread().isInterrupted())
        {

        }
        System.out.println("end1");
    }
}

class Thread2 extends Thread{
    public void run()
    {
        try {
            while (!this.currentThread().isInterrupted()) {
                this.sleep(10000);
            }
            System.out.println("end");
        } catch (InterruptedException e) {
            System.out.println("eeee");
        } finally {
            System.out.println("finally");
        }
    }
}

class Test{
    public static void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        System.out.println("hello");
        threadOne.start();
        threadTwo.start();
    }
}
```

运行命令启动程序

```
 javac test.java && java Test
```

可以发现程序在阻塞运转

## 14.2 中断线程

当线程的run函数方法执行到方法体最后一条语句的时候，并且经过return返回的时候，或者出现
在方法中没有捕获的异常的时候，线程就会终止

interrupt方法可以用来请求终止线程。

当对一个线程调用interrupt的时候，线程中断状态会被设置。这是每一个线程都具有的一个布尔值，每个线程可以通过检查这个值来判断线程
是否被中断了。

如果线程被阻塞了的线程调用interrupt的时候，调用阻塞将会被Interrupt Exception 中断，当然也存在不能被中断的调用

来看一下书中的说明，几个重要的函数

1.void interrupt()

向线程发送中断请求。线程的中断状态位置将会变成true。如果线程被sleep调用将会抛出异常，InterruptException 异常抛出。

2.static boolean interrupted()

测试当前线程是否被中断。注意这是一个静态方法，调用后会产生一个副作用，当前线程的中断状态将被设置为false

3.boolean isInterrupted

这一个调用不会产生中断的副作用

下面写一些demo方面的演示吧：

调用interrupt会修改中断对应线程的中断标志位

```

class Thread1 extends Thread{
    public void run()
    {
        while(!this.currentThread().isInterrupted())
        {

        }
        System.out.println("end1");
    }
}

class Thread2 extends Thread{
    public void run()
    {
        try {
            while (!this.currentThread().isInterrupted()) {
                this.sleep(10000);
            }
            System.out.println("end");
        } catch (InterruptedException e) {
            System.out.println("eeee");
        } finally {
            System.out.println("finally");
        }
    }
}

class Test extends Thread{
    public static   void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        System.out.println("hello");
        threadOne.start();
        threadTwo.start();
        try {
            sleep(1);//秒
        } catch (InterruptedException exception) {
            System.out.println("exception");
        }
        threadTwo.interrupt();
    }
}
```

第二个如果调用interrupt会吧isInterrupt修改为false

```

class Thread1 extends Thread{
    public void run()
    {
        while(!this.currentThread().isInterrupted())
        {

        }
        System.out.println("end1");
    }
}

class Thread2 extends Thread{
    public void run()
    {
            while (!this.currentThread().isInterrupted()) {
            }
            System.out.println("end");
    }
}

class Test extends Thread{
    public static   void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        System.out.println("hello");
        threadOne.start();
        threadTwo.start();
        threadOne.interrupt();
        threadTwo.interrupt();
    }
}
```

## 14.3 线程状态

线程有6个状态

    1.New(新创建)
    2.Runnable(可运行)
    3.Blocked(被阻塞)
    4.Waiting(等待)
    5.Timed waiting(计时等待)
    6.Terminated(被终止)
    
确定一个线程的当前状态，可调用getState方法

1.新创建的线程，还没有运行

例子程序

```
class Test extends Thread{
    public static   void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        System.out.println(threadOne.getState());
        System.out.println(threadTwo.getState());
    }
}
```

展示结果

```
NEW
NEW
```

2.可运行状态，调用start之后会变为run

```
class Test extends Thread{
    public static   void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        threadOne.start();

        System.out.println(threadOne.getState());
        System.out.println(threadTwo.getState());
    }
}
```

展示结果

```
RUNNABLE
NEW
```

3.被阻塞线程和等待线程

当线程处于被阻塞或者等待状态时候，它暂时不活动。他不运行并且资源消耗最少。直到线程调度器激活它。

1）尝试获取内部对象锁，而不是java.util.concurrent



2）等待线程Object.wait 或者 Thread.join，或者是等待java.util.concurrent库中的Lock或者Condition时候
```
import java.util.concurrent.locks.ReentrantLock;
class Thread1 extends Thread{

    public ReentrantLock lock;



    public void run()
    {
        lock.lock();
        System.out.println("lock1");
        while(!this.currentThread().isInterrupted())
        {

        }
        System.out.println("end1");
    }
}

class Thread2 extends Thread{

    public ReentrantLock lock;

    public void run()
    {
        lock.lock();
        System.out.println("lock2");
        while (!this.currentThread().isInterrupted()) {
        }
        System.out.println("end");
    }
}

class Test extends Thread{
    public  static void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        ReentrantLock mutex = new ReentrantLock();
        threadOne.lock = mutex;
        threadTwo.lock = mutex;
        threadOne.start();
        threadTwo.start();
        try {
            sleep(3);//秒
        } catch (InterruptedException exception) {
            System.out.println("exception");
        }

        System.out.println(threadOne.getState());
        System.out.println(threadTwo.getState());
    }
}
```

展示现象：
线程2进入waiting
```
lock1
RUNNABLE
WAITING

```
3)计时等待，带有超时的 sleep、join、wait等都会变为计时等待状态。

```
import java.util.concurrent.locks.ReentrantLock;
class Thread1 extends Thread{

    public ReentrantLock lock;



    public void run()
    {
        try {
            sleep(30);//秒
        } catch (InterruptedException exception) {
            System.out.println("exception");
        }
    }
}

class Thread2 extends Thread{

    public ReentrantLock lock;

    public void run()
    {
        try {
            sleep(30);//秒
        } catch (InterruptedException exception) {
            System.out.println("exception");
        }
    }
}

class Test extends Thread{
    public  static void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        ReentrantLock mutex = new ReentrantLock();
        threadOne.lock = mutex;
        threadTwo.lock = mutex;
        threadOne.start();
        threadTwo.start();
        try {
            sleep(3);//秒
        } catch (InterruptedException exception) {
            System.out.println("exception");
        }

        System.out.println(threadOne.getState());
        System.out.println(threadTwo.getState());
    }
}
```

4)线程终止

```
import java.util.concurrent.locks.ReentrantLock;
class Thread1 extends Thread{
    public void run()
    {
    }
}

class Thread2 extends Thread{
    public void run()
    {
    }
}

class Test extends Thread{
    public  static void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        threadOne.start();
        threadTwo.start();
        try {
            sleep(3);//秒
        } catch (InterruptedException exception) {
            System.out.println("exception");
        }

        System.out.println(threadOne.getState());
        System.out.println(threadTwo.getState());
    }
}
```

书中的一些函数:

join 等待指定的线程终止

join(long millis) 等待线程终止的毫秒数

## 14.4 线程的属性

1) 线程优先级

void setPriority(int newPriority)

设置线程的优先级，必须要在Thread.MIN_PRIORITY 和 Thread.MAX_PRIORITY之间

static int MIN_PRIORITY
值为1

static int NORM_PRIORITY
值为5

static int MAX_PRIORITY
值为10

yield()

让出当前线程的执行权

2）守护线程

不用多说了pthread_detach,c++里的实现就是pthread_detach(pthread_self()),让线程分离
可以自己释放，java中一定要在start之前设置

```
class Test extends Thread{
    public  static void main(String[] args)
    {
        Thread1 threadOne = new Thread1();
        Thread2 threadTwo = new Thread2();
        threadTwo.setPriority(10);
        threadOne.setDaemon(true);
        threadTwo.setDaemon(true);
        threadOne.start();
        threadTwo.start();
        System.out.println(threadOne.getState());
        System.out.println(threadTwo.getState());
    }
}
```

3）未捕获的异常处理器

线程的run方法不能抛出任何被检测的异常，但是不被检测的异常就会被终止，线程就会死亡

在线程死亡之前，异常传递一个用于未捕获线程的处理器,主要是Thread.UncaughtExceptionHandle

void uncaughtException(Thread t, Throwable e)

当然我们也可以使用setDefaultUncaughtExceptionHandler方法为线程安装一个处理器，替换处理器可以使用日志API发送错误日志。

如果不设置处理器，那么处理器是线程的ThreadGroup

ThreadGroup 类实现Thread.UncaughtExceptionHandler接口。它的uncaughtException方法如下：

如果该线程组有父辈线程组，那么父辈线程组的uncaughtException被调用

否则如果Thread.getDefaultUncaughtExceptionHandler返回一个非空的处理器，则调用这个处理器

否则如果Throwable如果是ThreadDeath一个实例，什么都不做

否则Throwable被输出到System.err上