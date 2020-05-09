#第五章 Posix 消息队列

###5.1 概述

消息队列可认为是一个消息链表。有足够写权限的线程可以往消息队列中放置消息，有足够读权限的现场可从队列中取走消息.每一个消息都是一个记录，他由发送者赋予一个优先级.在某个进程往一个消息队列写入消息之前，它并不需要另外某个进程在这个队列上等待消息的到达。这个跟管道的fifo是相反的，堆后两者来说，除非读出者已经存在，否则先有写入者没有任何意义。

本章节讲述Posix消息队列，第六章讲述System V消息队列。这两组函数之间存在着很深的共性，下面是差别：

1)Posix消息队列的读总是返回最高优先级的最早消息，堆System V消息队列的读则返回任意指定优先级的消息.

2)当往一个空队列防止一个消息的时候，Posix消息队列悄悄产生一个信号启动一个线程，system V消息队列则不提供这个机制.

队列的每一个消息具有下面的属性：

1）一个无符号整数优先级或一个长整数类型

2）消息的数据部分长度（可以为0）

3）数据本身（如果长度大于0）


我们可以把它想象成一个链表，链表中有两个属性：队列中允许的最大消息数以及每个消息的最大大小

我们开始使用一种新的程序设计，他在以后讨论消息队列、信号量和共享内存的章节里也会介绍到.

###5.2 mq_open、mq_close和mq_unlink函数

mq_open函数创建一个新的消息队列或打开一个已经存在的消息队列

	#include <mqueue.h>
	
	mqd_t mq_open(const char* name,int oflag,.../*mode_t mode,struct mq_attr *attr*/);
	

成功返回消息队列的描述符，若出错则返回-1

我们已经在2.2里面描述了name的规则，oflag参数是O_RDONLY,OWRONLY,或者O_RDWR之一，可能按位或上O_CREAT、O_EXCL或者O_NONBLOCK。我们已经在2.3讲过这些标志

当实际操作是创建一个新的队列的时候（已经制定O_CREAT标志，且锁请求的消息队列尚未存在），mode和attr参数是需要的。我们在图2-4中给出了mode值。attr参数用于给新队列指定某些属性，如果他们是空指针，那就使用默认属性

mq_open的返回值我们成塔为消息队列描述符，他不必是向文件描述符那样的短整数，这个值用作其余7个消息队列函数的第一个参数.


已经打开的消息队列是由mq_close 关闭的。

	#include <mqueue.h>
	
	int mq_close(mqd_t mqdes);
	
其功能与关闭一个已经打开的文件close函数类似：调用进程可以不在使用这个描述符，但是其消息队列并不在系统中删除，一个进程终止的时候，它所有打开这的消息队列都关闭，就像调用mq_close 一样

要从系统中删除作用mq_open的第一个参数的某个name，必须调用mq_unlink

	#include <mqueue.h>
	
	int mq_unlink(const char *name);
	
	成功返回0，如果出错返回-1
	
每个消息队列有一个保存其当前打开这描述符的引用计数器，因而本函数能够实现类似与unlink函数删除一个文件的机制：当一个消息队列的引用技术仍然大于0的时候，他的name就能删除，但是这个队列的析构要到最后一个mq_close 的时候才进行。

posix消息队列至少具有随内核的持续性。也就是说，即使当前没有进程打开这个消息队列，这个队列的消息一直存在，一直到调用mq_unlink并让他引用技术达到0以删除这个队列为止.

例子程序：

	int main(int argc,char **argv)
	{
	    int c,flags;
	    mqd_t mqd;

	    flags = O_RDWR | O_CREAT;

	    while ((c = getopt(argc,argv,"e")) != -1)
	    {
		switch (c)
		{
		    case 'e':
		        flags |= O_EXCL;
		}
	    }

	    if(optind != argc-1)
	    {
		printf("usage:mqcreate [ -e ] <name>");
		exit(-1);
	    }

	    mqd = mq_open(("/tmp.1234"),flags,FILE_MODE,NULL);
	    printf("%d\n",errno);
	    printf("%s\n",strerror(errno));
	    printf("%d\n",mqd);

	    sleep(5);
	    mq_close(mqd);
	    exit(0);
	    return 0;
	}
	
注意好多linux 程序都只能出现一个斜杠"/"

mq_unlink程序:

	int main(int argc,char **argv)
	{
	    mq_unlink("/tmp.1234");
	    return 0;
	}
	
删除已经创建的消息队列文件.

###5.3  mq_getattr 和 mq_setattr

每个消息队列有4个属性，mq_getattr返回所有属性，mq_setattr则设置某个属性

	#include <mqueue.h>

	int mq_getattr(mqd_t mqdes,struct mq_attr *attr);
	int mq_setattr(mqd_t mqdes,struct mq_attr *attr,struct mq_attr *oattr);
	
成功返回0失败就返回-1

mq_attr含有下面的属性:

	struct mq_attr{
		long mq_flags;/*message queue flag:0,O_NONBLOCK*/
		long mq_maxmsg;/*max number of messages allowed on queue*/
		long mq_msgsize;/*消息的最大字节数*/
		long mq_curmsgs;/*队列中当前消息数*/
	}
	
指向某个mq_attr结构的指针可作为mq_open的第四个参数传递，从而允许我们在这个函数的实际操作是创建一个新的队列的时候，给它指定mq_maxmsg和mq_msgsize 属性。mq_open忽略该结构的另外两个成员.

mq_getattr把所指定队列的当前属性填入attr所指向的结构

mq_setattr给所指定队列设置属性，但是只由attr指向的mq_attr结构的mq_flags成员，已设置或清除非阻塞标志位.这个结构的另外三个成员被忽略：每个队列的最大消息数和每个消息的最大字节数只能在创建队列的时候设置，队列中当前的消息数只能获取不能设置

如果oattr指针非空，那么所指向队列的先前属性和当前状态将返回到由这个指针指向的结构体中(mq_flags、mq_maxmsg和mq_msgsize)和当前状态(mq_curmsgs)将返回到结构体指针中


####5.4 mq_send 和 mq_receive 函数

这两个函数分别用于往一个队列中放置一个消息和从一个队列中取走一个消息.每个消息有一个优先级，他是一个小于MQ_PRIO_MAX的无符号整数.Posix要求这个上限至少为32.

mq_receive 总是返回制定队列中最高优先级的最早消息，而且这个优先级随消息内核以及其长度一同返回.

	#include <mqueue.h>
	
	int mq_send(mqd_t mqdes,const char* ptr,size_t len,unsigned int prio);
	
	ssize_t mq_receive(mqd_t mqdes,char *ptr,size_t len,unsigned int priop);
	
这两个函数的前三个参数和read和write的前三个参数类似

	mq_receive 的 len 参数值不能小于能加到所指定队列中的消息的最大大小.要是len小于这个值，mq_receive就立即返回EMSGSIZE错误.
	
	这就意味着使用posix消息队列的大多数应用程序必须在打开某个队列后调用mq_getattr确定最大消息的大小，然后分配一个或多个那样大小的缓冲区。通过要求每个缓冲区总是足以存放队列中的任意消息，mq_receive不必返回消息是否大于缓冲区的通知。作为比较的例子，system V消息队列可能使用MSG_NOERROR标志，返回E2BIG错误，接收UDP数据报的recvmsg函数可以使用MSG_TRUNC标志.
	
	mq_send的prio参数是待发送消息的优先级，其值必须小于MQ_PRIO_MAX。如果mq_receive的priop参数是一个非空指针，所返回消息的优先级就通过这个指针存放.如果应用不必使用优先级不通的消息，那就通过给mq_send指定值为0的优先级，给mq_receive制定一个空指针作为其中的最后一个参数.
	
	0字节长度消息是允许的.因此返回值为0的则表示长度为0的消息.
	
	
mq_send的程序:

	int main(int argc,char **argv)
	{
	    mqd_t mqd;
	    void *ptr;

	    size_t  len;

	    uint prio;

	    if(argc != 4)
	    {
		printf("usage:mqsend <name> <#bytes> <priority>");
		exit(-1);
	    }

	    len = atoi(argv[2]);

	    prio = atoi(argv[3]);

	    mqd = mq_open(argv[1],O_WRONLY);

	    ptr = calloc(len,sizeof(char));

	    mq_send(mqd,ptr,len,prio);
	    return 0;
	}
	
	
例子：mqreceive 程序

	int main(int argc,char **argv)
	{
	    int c,flags;

	    mqd_t mqd;

	    ssize_t n;
	    uint prio;

	    void *buff;

	    struct mq_attr attr;

	    flags = O_RDONLY;

	    while((c = getopt(argc,argv,"n")) != -1)
	    {
		switch (c)
		{
		    case 'n':
		        flags |= O_NONBLOCK;
		        break;
		}
	    }

	    if(optind != argc-1)
	    {
		printf("usage: mqreceive [-n] <name>");
		exit(0);
	    }

	    mqd = mq_open(argv[optind],flags);

	    mq_getattr(mqd,&attr);

	    buff = malloc((size_t)attr.mq_msgsize);

	    n = mq_receive(mqd,buff,(size_t)attr.mq_msgsize,&prio);

	    printf("read %ld bytes,priority = %u\n",(long)n,prio);
	    exit(0);
	}
	
运行结果

	zhanglei@zhanglei-OptiPlex-9020:~/ourc/test$ gcc msg_create.c -o msg_create -lrtzhanglei@zhanglei-OptiPlex-9020:~/ourc/test$ ./msg_create /test1
	0
	Success
	3
	zhanglei@zhanglei-OptiPlex-9020:~/ourc/test$ ./msg_send /test1 100 6
	zhanglei@zhanglei-OptiPlex-9020:~/ourc/test$ ./msg_send /test1 33 18
	zhanglei@zhanglei-OptiPlex-9020:~/ourc/test$ ./msg_receive /test1
	read 33 bytes,priority = 18
	zhanglei@zhanglei-OptiPlex-9020:~/ourc/test$ ./msg_receive /test1
	read 100 bytes,priority = 6
	zhanglei@zhanglei-OptiPlex-9020:~/ourc/test$ 

###5.5 消息队列的限制

我们已经遇到任意给定队列的两个限制，他们都是在创建队列的时候建立的

mq_mqxmsg 队列的最大消息数

mq_msgsize 给定消息的最大字节数

消息队列的实现定义了另外两个限制 

MQ_OPEN_MAX 一个进程能够同时拥有的打开着消息队列的最大数目为8

MQ_PRIO_MAX 任意消息的最大优先级值+1 POSIX要求他至少是32

这两个值定义在unistd.h的头文件中

输出代码

	printf("MQ_OPEN_MAX = %ld,MQ_PRIO_MAX = %ld\n",sysconf(_SC_MQ_OPEN_MAX),sysconf(_SC_MQ_PRIO_MAX));
	
###5.6  mq_notify函数

我们在第六个章节中讨论过system v 的消息队列的问题之一是无法通知一个进程何在某个队列中放置一个消息.我们可以阻塞在msgrcv的调用中,但是那将阻止我们在等待期间做其他任何事情。如果给msgrcv指定非阻塞标志，那么尽管不阻塞了，但是必须持续调用这个函数以确定何时有一个消息到达。我们说过的这杯称为轮询，是对cpu的一种浪费。我们需要一种方法，让系统告诉我们何时有一个消息放置到空的队列中。

posix消息队列允许异步事件通知，以告诉我们何时有一个消息放置到空的消息队列中。这种通知方式有两种选择.这种通知方式有两种方式可以提供选择

产生一个信号
创建一个线程来执行一个指定的函数
	
通过调用mq_notify函数

	#include <mqueue.h>
	
	int mq_notify(mqd_t mqdes,const struct sigevent *notification);
	
这个函数为指定队列建立或删除异步事件通知


sigevent结构是随Posix.1实时信号加新的，后者将在下一个环节详细讨论。这个结构以及本章中引入的所有新的信号相关常值都定义在signal.h这个头文件中.

	union sigval{
		int sival_int;
		void *sival_ptr;
	};

	struct sigevent{
		int sigev_notify;
		int sigev_signo;
		union sigval sigev_value;
		void (*sigev_notify_function)(union sigval	);
		pthread_attr_t *sigev_notify_attributes;
	};

1)如果notification参数不是空的，那么当前进程希望在有一个消息到达所指定的先前为空的消息队列及时得到通知。我们说这个进程被注册为接收这个消息队列的通知

2）如果notification参数是空的，而且当前进程目前被注册为接收消息队列的通知，那么已经存在的注册将要被撤销

3）任意一个时刻只有一个进程可以被注册为接收某个给定队列的通知.

4）当有一个消息到达某个先前为空的队列，而且已经有一个进程被注册为接收这个队列的通知的时候，只有在没有任何线程阻塞在这个队列的mq_receive调用中的前提下，通知消息才会发出。这就是mq_receive调用中的阻塞比任何通知的注册都优先。

5）当这个通知被发送给它的注册进程时候，通知被注销，必须再次调用mq_notify然后重新注册。

例子:

	mqd_t mqd;
	struct mq_attr attr;
	struct sigevent sigev;

	static void sig_usr1(int);
	void *buff;
	int main(int argc,char **argv)
	{

	    int flags = O_RDONLY;
	    mqd = mq_open("/test1",flags);

	    mq_getattr(mqd,&attr);

	    buff = malloc((size_t)attr.mq_msgsize);

	    signal(SIGUSR1,sig_usr1);
	    sigev.sigev_notify = SIGEV_SIGNAL;
	    sigev.sigev_signo = SIGUSR1;

	    mq_notify(mqd,&sigev);

	    for(;;)
		pause();
	    exit(0);
	}

	static void sig_usr1(int signo)
	{
	    ssize_t n;
	    mq_notify(mqd,&sigev);

	    n = mq_receive(mqd,buff,(size_t)attr.mq_msgsize,NULL);
	    printf("SIGUSR1 received,read %ld bytes\n",(long)n);
	    return;;
	}
	
简单的通知:

在深入探讨posix实时信号和线程之前，我们可编写一个简单的程序，当有一个消息放入到空的队列的时候，这个程序产生一个SIGUSR1的信号

声明全局变量

打开队列，取得属性，分配读取缓冲区列表

建立信号通知程序，在sigevent结构的sigev_notify成员中填入SIGEV_SIGNAL常值,其意思是当所指定队列由空变为非空的时候，我们希望有一个信号会产生。将sigev_signo成员设置成希望产生的信号，然后调用mq_notify。

无限循环

main函数无限休眠，调用pause中的循环，函数每一次捕捉到信号都会产生-1

捕捉信号，读出消息

mq_notify 以便为下一个事件重新注册，然后读出消息并输出其长度。本程序中我们忽略了接收到消息的优先级。

测试结果:

	中途出现了是mq_send出现了阻塞，后来推测出是缓冲区满掉了
	
	如果同时两个进程监控会出现EBUSY错误所以说两个进程监控本身在这种模式下是不合理的。
	
####5.6.2 Posix信号:异步信号安全函数

图5-9程序的问题是他从信号处理程序中调用mq_notify、mq_receive和printf。这些函数实际上都不从信号处理程序中调用。

posix使用异步信号处理程序中需要注意函数的安全性。

具体见5-10中的异步函数列表

###5.6.3 例子：信号通知

避免从信号处理程序中调用任何函数的方法之一是：让处理程序仅仅设置一个全局标志，由某个线程检查这个标志以确定何时收到一个消息，不过它另外含有一个错误



	#define SLEN 20
	#define MAXLEN 50

	#include "mesg.h"
	#include <ucontext.h>
	#define FIFO1   "/tmp/fifo.1"
	#define FIFO2   "/tmp/fifo.2"
	#define FILE_MODE 0666
	void client(int readfd,int writefd);
	void server(int readfd,int writefd);

	static void sig_usr1(int signo);

	volatile sig_atomic_t mqflag;

	int main(int argc,char **argv)
	{

	    mqd_t mqd;
	    struct mq_attr attr;
	    struct sigevent sigev;

	    void *buff;

	    size_t n;

	    sigset_t zeromask,newmask,oldmask;

	    mqd = mq_open("/test1",O_RDONLY);

	    mq_getattr(mqd,&attr);

	    buff = malloc((size_t)attr.mq_msgsize);

	    //初始化信号集
	    sigemptyset(&zeromask);
	    sigemptyset(&newmask);
	    sigemptyset(&oldmask);

	    sigaddset(&newmask,SIGUSR1);

	    signal(SIGUSR1,sig_usr1);

	    sigev.sigev_notify = SIGEV_SIGNAL;
	    sigev.sigev_signo = SIGUSR1;

	    mq_notify(mqd,&sigev);

	    for(;;)
	    {
		//用于改变进程的当前阻塞信号集,也可以用来检测当前进程的信号掩码。
		/**
		 * 一个进程的信号屏蔽字规定了当前阻塞而不能递送给该进程的信号集。
		 * sigprocmask()可以用来检测或改变目前的信号屏蔽字，其操作依参数how来决定，
		 * 如果参数oldset不是NULL指针，那么目前的信号屏蔽字会由此指针返回。如果set是一个非空指针，
		 * 则参数how指示如何修改当前信号屏蔽字。每个进程都有一个用来描述哪些信号递送到进程时将被阻塞的信号集，
		 * 该信号集中的所有信号在递送到进程后都将被阻塞。
		 */

		sigprocmask(SIG_SETMASK,&newmask,&oldmask);

		while(mqflag == 0)
		    sigsuspend(&zeromask);

		mqflag = 0;

		mq_notify(mqd,&sigev);

		n = mq_receive(mqd,buff,attr.mq_msgsize,NULL);

		printf("read %ld bytes\n",(long)n);

		//unblock SIGUSR1
		sigprocmask(SIG_UNBLOCK,&newmask,NULL);
	    }
	    exit(0);
	}

	static void sig_usr1(int signo)
	{
	    mqflag = 1;
	    return;;
	}
	
程序分析：

设置全局变量：

	既然信号处理程序执行的唯一操作是把mqflag设置为非0，于是图5-9中的全局变量仍然是全局变量。降低全局变量的数目肯定是一个非常好的技巧，尤其是使用线程更是如此
	
打开消息队列：

	打开通过命令行参数指定消息队列，获取其属性，然后分配一个缓冲区.
	
初始化信号集：

	初始化三个信号集，并且在newmask集中打开对应的SIGUS1的位.
	
建立信号处理程序启用通知

	给SIGUSR1建立一个信号处理程序，填写sigevent结构，调用mq_notify。
	
等待信号处理程序设置标志

	调用sigprocmask阻塞SIGUSR1，并把当前信号掩码保存到oldmask中。随后在一个循环中测试全局变量mqflag，以等待信号处理程序将它设置为非零。只要它为0，我们就调用sigsuspend，它原子性地将调用线程投入睡眠，并把它的信号掩码复位成为zeromask（没有一个信号被阻塞）.APUE的10.16节详细讨论sigsuspend以及为什么只能在SIGUSR1被阻塞时候测试mqflag变量。每次sigsuspend返回的时候SIGUSR1被重新阻塞。
	
重新注册并读出消息

	当mqflag为非0的时候，重新注册并从队列中读出消息，随后给SIGUSR1解阻塞并返回for循环顶部
	
缺陷：

	可能在存在多个消息通知的时候被忽略掉消息，只会返回一个消息
	
	
####5.6.5例子：使用sigwait代替信号处理程序的通知

上一个例子尽管正确，但效率还可以更高些。我们程序通过调用sigsusoend阻塞，以等待某个消息的到达。当有一个消息被放置到某个空的队列中的时候，这个信号产生，主线程被阻止，信号处理程序执行并且设置mqflag变量，主线程再次执行，发现mq_flag为非零，于是读出这个消息，更加简单的办法之一是阻塞在某个函数中，等待这个信号的交替，而不是让内核职位设置一个标志的信号处理程序。sigwait提供了这个能力

	#include <signak.h>
	
	int sigwait(const sigset_t *set,int *sig);
	
调用sigwait前，我们阻塞某个信号集。我们将这个信号集指定为set参数。sigwait然后一直阻塞到这些信号有一个或者多个待处理，这时候它返回其中一个信号。这个信号值通过指针sig存放，函数的返回值则为0.这个过程被称为“同步的等待一个异步事件”：我们是在使用信号，但没有涉及到异步信号处理程序。

demo:

	#include "mesg.h"
	#include <signal.h>


	int main(int argc,char **argv)
	{
	    sigset_t newmask;
	    int signo;
	    mqd_t  mqd;
	    void *buff;
	    ssize_t n;

	    struct mq_attr attr;
	    struct sigevent sigev;

	    mqd = mq_open("/test1",O_RDONLY);


	    if(mqd <= 0 && errno != EEXIST)
	    {
		printf("create message list error,errno:%d\n",errno);
		exit(-1);
	    }

	    mq_getattr(mqd,&attr);

	    //申请一个缓冲区
	    buff = malloc((size_t)attr.mq_msgsize);

	    sigemptyset(&newmask);

	    sigaddset(&newmask,SIGUSR1);

	    sigprocmask(SIG_BLOCK,&newmask,NULL);

	    sigev.sigev_signo = SIGUSR1;
	    sigev.sigev_notify = SIGEV_SIGNAL;

	    mq_notify(mqd,&sigev);

	    for(;;)
	    {
		sigwait(&newmask,&signo);

		if(signo == SIGUSR1)
		{
		    mq_notify(mqd,&sigev);

		    while((n = mq_receive(mqd,buff,attr.mq_msgsize,NULL)) >= 0)
		    {
		        printf("read %ld bytes\n",(long)n);
		    }

		    if(errno == EAGAIN)
		    {
		        printf("mq_receive error\n");
		        exit(-1);
		    }
		}
	    }
	}	
	
	
初始化信号集并阻塞SIGUSR1

把某个信号集初始化成只包含有SIGUSR1,然后用sigprocmask阻塞这个信号

等待信号

在sigwait调用中阻塞并且等待这个信号，SIGUSR1被递交后，重新注册所有的消息。

####5.6.6使用select的posix消息队列

demo:

	#include "mesg.h"
	#include <signal.h>

	int pipefd[2];

	static void sig_usr1(int);

	int main(int argc,char **argv)
	{
	    int nfds;
	    char c;
	    fd_set rset;
	    mqd_t mqd;
	    void *buff;
	    ssize_t n;
	    struct mq_attr attr;
	    struct sigevent sigev;

	    mqd = mq_open("/test1",O_RDONLY);

	    signal(SIGUSR1,sig_usr1);

	    sigev.sigev_notify = SIGEV_SIGNAL;
	    sigev.sigev_signo = SIGUSR1;

	    mq_notify(mqd,&sigev);

	    FD_ZERO(&rset);

	    for(;;)
	    {
		FD_SET(pipefd[0],&rset);

		nfds = select(pipefd[0]+1,&rset,NULL,NULL,NULL);

		if(FD_ISSET(pipefd[0],&rset))
		{
		    read(pipefd[0],&c,1);

		    mq_notify(mqd,&sigev);

		    while((n = mq_receive(mqd,buff,attr.mq_msgsize,NULL)) >= 0)
		    {
		        printf("read %ld\n",n);
		    }

		    if(errno == EAGAIN)
		    {
		        printf("mq_receive error\n");
		        exit(-1);
		    }
		}

	    }


	}

	static void sig_usr1(int signo)
	{
	    write(pipefd[1],"", sizeof(""));
	    return;
	}
	
###5.7 Posix实时信号：

利用unix 提供的可靠信号模型

信号可划分为两个大组。

1）其值在SIGRTMIN和SIGRTMAX之间的试试信号。Posix要求至少提供RTSIG_MAX种实时信号，然而该常值的最小值为8.

2）所有其他信号：SIGALRM、SIGINT、SIGKILL等

术语实时行为隐藏着下面的特征：

1）信号是排队的。这就是说	，如果同一个信号产生三次，他就会被递交三次。另外，一种给定信号的多次发生以先进先出顺序排队。我们不就就给出一个信号排队的例子。对于不排队的信号产生三次某种信号我们只会递交一次。

2）当产生多个SIGRTMIN到SIGRTMAX范围内的解阻塞信号排队的时候，值较小的信号先于值较大的信号递交

3）当某个非实时信号提交的时候，传递给他的信号处理程序的唯一参数是该信号的值。实时信号比其他信号携带更多的信息。通过设置SA_SIGINFO标志安装的任意实时信号的信号处理声明如下

typedef struct{
	int si_signo;
	int si_code;
	union sigval si_value;
}siginfo_t;

4)一些新函数定义成使用实时信号的工作。例如sigqueue函数代替kill函数向某个进程发送信号，该函数允许随信号传递一个sigval联合。


输出实时信号值:

输出最小和最大的实时信号值，以查看系统实现支持多少种实时信号。我们把这两个常值转化为一个证书，因为实现这两个常值定义为调用sysconfig的宏

fork子进程阻塞三种实时信号

派生一个子进程，由子进程调用sigprocmask阻塞我们使用的三种实时信号：SIGRTMAX、SIGRTMAX-1、SIGRTMAX-2

建立信号处理程序

调用signal_rt函数，建立我们的sig_rt函数来作为这三种实时信号的处理程序。该函数设置SA_SIGINFO标志，再加上三种信号都是实时信号，于是我们预期他具有实时行为。这个函数还设置执行信号处理程序期间需要阻塞的信号掩码。

等待父进程产生信号，然后解除阻塞信号

等待6秒以允许父进程产生预定的9个新号。然后在调用sigprocmask解除阻塞那三种实时信号。该操作应允许所有排队的信号都被提交。子进程停顿三秒，以便信号处理程序调用printf9次，然后停止。

父进程发送9个信号

父进程停顿三秒，以便子进程阻塞所有信号。父进程随后给三种实时信号的第一种产生三个信号：i取这三种实时信号的值，对于每个i值，j取值为0\1和2.我们特意从最高的信号值开始产生信号，因为期待他们从最低的信号值开始递交。我们还伴随每个信号发送一个不同的整数值，以验证一种给定信号的3次发生是按FIFO的顺序产生的。

信号处理程序

我们的信号处理程序只输出所递交信号的有关信息。

demo:

	#include "mesg.h"
	#include <signal.h>

	int pipefd[2];
	typedef void (sigfunc_t)(int signo,siginfo_t *info,void *context);

	sigfunc_t* signal_rt(int signo,sigfunc_t* func,sigset_t *mask);
	static void sig_rt(int signo,siginfo_t *info,void *context);
	int main(int argc,char **argv)
	{
	    int i,j;

	    pid_t pid;

	    sigset_t newset;

	    union sigval val;

	    if((pid = fork()) == 0)
	    {
		sigemptyset(&newset);

		sigaddset(&newset,SIGRTMAX);
		sigaddset(&newset,SIGRTMAX-1);
		sigaddset(&newset,SIGRTMAX-2);

		sigprocmask(SIG_BLOCK,&newset,NULL);

		signal_rt(SIGRTMAX,sig_rt,&newset);
		signal_rt(SIGRTMAX-1,sig_rt,&newset);
		signal_rt(SIGRTMAX-2,sig_rt,&newset);

		sleep(6);

		sigprocmask(SIG_UNBLOCK,&newset,NULL);
		sleep(3);
		exit(0);
	    }

	    sleep(3);

	    //在队列中向指定进程发送一个信号和数据。
	    for(i = SIGRTMAX;i >= SIGRTMAX-2;i--)
	    {
		for(j=0;j<=2;j++)
		{
		    val.sival_int = j;
		    int res =sigqueue(pid,i,val);
		    printf("sent signal %d,val = %d,res:%d\n",i,j,res);
		}
	    }
	    exit(0);
	}

	static void sig_rt(int signo,siginfo_t *info,void *context)
	{
	   printf("received signal #%d,code = %d,ival=%d\n",signo,info->si_code,info->si_value.sival_int);
	}

	sigfunc_t* signal_rt(int signo,sigfunc_t* func,sigset_t *mask)
	{
	    struct sigaction act,oact;

	    act.sa_sigaction = func;
	    act.sa_mask = *mask;
	    act.sa_flags = SA_SIGINFO;

	    if(signo == SIGALRM) {
	#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
	#endif
	    }else{
	#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;
	#endif
	    }

	    if(sigaction(signo,&act,&oact)) {
		printf("error\n");
		return ((sigfunc_t*) SIG_ERR);
	    }
	    printf("ok\n");
	    return (oact.sa_sigaction);
	}
	
	
	
###小结

Posix消息队列比较简单：mq_open创建一个新队列或者打开一个已经存在的队列，mq_close 关闭队列，mq_unlink删除队列的名字。往一个队列中放置消息使用mq_send，从一个队列中读出消息使用mq_receive。队列属性的查询和设置使用mq_getattr和mq_setattr，函数mq_notify则允许我们注册一个信号或者线程，他们在有一个消息被放进来的时候会产生信号或线程。队列每个消息被富裕一个小整数优先级，mq_receive每次被调用时候总是返回优先级最高的消息。

我们引入posix实时信号，他们在SIGRTMIN和SIGRTMAX之间，当设置SA_SIGINFO标志来安装这些信号处理程序时候，1）这些信号是排队的,2）排队的信号按照FIFO顺序递交 3）给信号处理程序传递了两个额外参数

