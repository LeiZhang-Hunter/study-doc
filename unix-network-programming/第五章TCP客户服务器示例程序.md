# TCP客户/服务器示例程序


####1.概述
1)客户从标准输入读入一行文本，并且写给服务器

2)服务器从网络读入这行文本，回射给客户端

3)客户从网络输入读入这行回射文本并且显示在标准输出上

    ------------>TCP客户(write)-------------->(read)tcp服务端

    <------------TCP客户(readline)<-----------(write)TCP服务器

TCP连接是全双工的

我们更应该去思考一些问题？

1.客户端和服务端启动时会发生什么

2.客户正产终止时会发生什么

3.若服务器在客户之前终止会发生什么

4.服务器崩溃客户会发生什么等等

####2.简单的TCP服务端程序

    #include "unp.h"
    
    int main()
    {
        int listenfd,connfd;
    
        listenfd = socket(AF_INET,SOCK_STREAM,0);
    
        struct sockaddr_in cliaddr,serveraddr;
    
        serveraddr.port = htons(INADDR_ANY);
    
        serveraddr.sin_addr.s_addr = htonl(SERV_PORT);
    
        serveraddr.family = AF_INET;
    
        bind(listenfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr));
    
        lisen(listenfd,LISTENQ);
    
        for(;;)
        {
            connfd = accept(listenfd,(struct sockaddr *)cliaddr,sizeof(cliaddr));
            if((pid = fork()) == 0)
            {
                close(listenfd);
                str_echo(connfd);
                exit(0);
            }
            close(connfd);
        }
    }

这是一个非常简单的程序（自己都能不参考任何文件默写出来了。。）但是要注意close 陷阱，防止套接字泄漏

####3.str_echo

    #include "unp.h"
    
    void str_echo(int sockfd)
    {
        size_t n;
        char buf[MAXLINE];
    again:
        while((n = read(sockfd,buf,MAXLINE)) > 0)
            Writen(sockfd,buf,n);
        if(n < 0 && errno = EINTR)
            goto again;
        else if(n < 0)
            err_sys("read error\n");
    }

read读到数据后,write 回射给客户程序，除非进程收到FIN信号，read返回0则跳出循环，终止进程
  
####4.TCP客户程序

TCP的main函数

	int main(int argc,char **argv)
	{
		int sockfd;
		struct sockaddr_in servaddr;
		sockfd = socket(AF_INET,SOCK_STREAM,0);
		//清空服务器结构地址中的存储值
		bzero(&servaddr,sizeof(servaddr));
	
		servaddr.sin_family = AF_INET;
		servaddr.port = htons(SERV_PORT);
		inet_pton(AF_INET,argv[1],servaddr.sin_addr);
	
		connect(sockfd,(struct scokaddr*)&servaddr,sizeof(servaddr));
	
		str_cli(stdin,sockfd);
	
		exit(0);
	}

1.socket函数创建一个套接字

2.connect连接上一个 套接字

####5.TCP回射客户程序:str_cli函数

    #include "unp.h"

    void str_cli(FILE* fp,int sockfd)
    {
        char sendline[MAXLINE],recvline[MAXLINE];

        while(Fgets(sendline,MAXLINE,fp) != NULL){
            Writen(sockfd,sendline,strlen(sendline));
            if(Readline(sockfd,recvline,MAXLINE) == 0)
                err_quit("str_cli:server teminated prematurely");
            Fputs(recvline,stdout);
        }
    }

读一行，写到服务器

fgets读入一行文本，write把这一行发送给服务器

从服务器读入回射行，写入标准行

readline从服务器读入回射行,fputs把他写到标准输出

返回到main函数

当遇到文件结束或错误的时候，fgets返回一个空指针，于是客户处理循环终止。我们Fgets检查包裹函数是否发生错误，若发生错误则终止进程，因此Fgets遇到哦文件结束才会返回空指针

####6.正常启动

我们除了要弄清楚TCP的启动过程，更重要的是我们要弄清客户机主机崩溃、客户进程崩溃、网络连接断开，只有搞清楚边界条件以及它们与TCP IP的相互作用，我们才能写出健壮的程序

ps -t pts/6 -o pid,ppid,tty,stat,args,wchan(ps a -o pid,ppid,tty,stat,args,wchan )

WCHAN的对应:

linux阻塞于accept或connect时候，输出wait_for_connect

linux阻塞在输入输出的时候,输出tcp_data_wait

阻塞在中断Io的时候，输出read_chan


####7.正常终止

我们跑起来上面的程序后，输入文件结束符end of file 以control d 来结束,由于

while(Fgets(sendline,MAXLINE,fp) != NULL){ 

这一行已经返回NULL了，客户端程序已经直接exit了

exit进程终止是关闭进程中所有打开的文件描述符，因此客户端会给服务端发送一个FIN信号，服务器则用ACK来响应，至此，服务器进入了CLOSE_WAIT状态，客户进程处于FIN_WAIT_2状态

当TCP进程接收到FIN的时候，服务器子进程会调用readline，于是feadline返回0，str_echo返回服务器子进程的main函数

服务器子进程通过exit来终止

服务器子进程会在向客户端发送FIN信号，客户端回射ACK信号，至此结束连接，客户端进入TIME_WAIT状态

进程终止另一部分处理是，子进程会给父进程发送一个SIGCHILD的信号，如果忽略这个信号，子进程会进入僵尸状态

总结：

当fork中调用exit会出现两个流程

1.TCP四次捂手（因为关了所有的文件描述符）

2.发送一个SIGCHILD的信号

还有time_wait的产生，由于父进程没有对SIGCHILD信号做处理，所以最后导致客户端也处在了time_wait状态下

####8.POSIX信号处理

信号是指某个进程发送了某个通知，我们也称为软中断，信号通常是异步发生的，也就是说进程预先不知道信号的发生，signal函数提供了信号的捕获，但是有两个信号并不能被捕获，他们是SIGSTOP和SIGKILL信号，个别信号我们可以设置SIG_IGN来忽略他，但是 SIGSTOP和 SIGKILL不能被忽略，SIGCHILD和SIGURG默认是被忽略的

signal的实现:

    #include "unp.h"
    
    Sigfunc* signal(int signo,Sigfunc* func)
    {
        struct sigaction act,oact;

        act.sa_hander = func;

        sigemptyset(&act.sa_mask);

        act.sa_flags = 0;

        if(signo == SIGALRM)
        {
    #ifdef SA_INTERRUPT
            act.sa_flags |= SA_INTERRUPT;
    #endif
        }else{
    #ifdef     SA_AESTART
        act.sa_flags |= SA_RESTART;
    #endif
        }

        }

        if(sigaction(signo,&act,&oact) < 0)
            return SIG_ERR;

        return     oact.sa_handler;
    }


POSIX信号处理函数总结:

1.一旦安装了信号处理函数，它便会一直被安装

2.一个信号运行期间，正被递交的信号是阻塞的，

3.如果一个信号在阻塞的时候被提交多次，那么接触阻塞后通常只递交一次，也就是说unix信号不排队

4.利用sigprocmask函数选择性的阻塞一组信号是可能的，这使我们可以做到在一段临界区代码执行期间，防止捕捉某些信号，以此保护这段代码

####9.处理SIGCHILD函数

调用wait函数可以处理僵尸进程

函数如下定义:

    #include "unp.h"

    void sig_child(int signo)
    {
        pid_t pid;
        int stat;

        pid = wait(&stat);
        printf("child %d teminated\n",pid);
        return;
    }

    signal(SIGCHILD,sig_child);

具体处理过程如下：

我们键入EOF字符来终止客户。客户发送一个FIN给服务器，服务器响应一个ACK

收到客户的fin导致服务器递送一个EOF给readline，从而进程终止

当调用signal函数时，accept处于阻塞状态，sig_child函数调用wait取得子进程的pid和终止状态，调用pirntf，最后返回

既然信号被父进程捕获，内核会返给accept 一个 EINTR错误，而父进程不处理该错误，于是终止

慢系统规则是：

当某个慢系统调用一个进程捕获某个信号且相应信号处理函数返回时，系统可能返回一个EINTR错误。有些内核重启某个被中断的系统调用，为了方便移植，我们必须对慢系统调用signal返回EINTR有所准备。即使某个实现支持SA_RESTART，并非所有的中断系统都可以自动重启，为了对中断做处理我们必须在程序中做如下处理：

    for(;;){
        clien = sizeof(cliaddr);
        if(connfd = accept(listenfd,(SA*)&cliaddr,sizeof(cliaddr)) < 0){
            if(errno == EINTR)
                continue;
            else
                err_sys("accept error\n");
        }
    }

####10.wait和waitpid

	#include <sys/wait.h>
	pid_t wait(int *statloc);
	pid_t waitpid(pid_t pid,int *statloc,int options);

wait和waitpid 均只返回两个值终止进程的pid,通过statloc指针返回子进程的终止状态，我们可以调用三个宏来检查终止状态，并且辨别子进程是否是正常终止、或者是由某个信号杀死还是由于仅仅因为作业停止。另外有些宏用于接着获取子进程的推出状态、杀死子进程的信号值或者停止作业控制的信号值。我们将为此目的使用宏WIFEXITED和WEXITSTATUS

如果调用wait没有已经终止的子进程，不过由一个或者多个子进程仍在执行，那么wait将阻塞到现有子进程第一个终止为止

wait pid函数就等待哪个进程以及是否阻塞给了我们更多的控制，首先第一个参数pid我们可以指定等待哪个进程，如果值是-1表示将会等待第一个终止的子进程，option选项允许我们指定附加项，最常用的是WNOHANG，它告知内核没有已经终止的子进程的时候不要阻塞

wait和waitpid的区别

	int main(int argc,char **argv)
	{
		int i,sockfd[5];
		struct sockaddr_in servaddr;
		for(i = 0;i<5;i++){
			sockfd[i] = socket(AF_INET,SOCK_STREAM,0);
			//清空服务器结构地址中的存储值
			bzero(&servaddr,sizeof(servaddr));
		
			servaddr.sin_family = AF_INET;
			servaddr.port = htons(SERV_PORT);
			inet_pton(AF_INET,argv[1],servaddr.sin_addr);
		
			connect(sockfd[i],(struct scokaddr*)&servaddr,sizeof(servaddr));
		}
	
		str_cli(stdin,sockfd[0]);
	
		exit(0);
	}

当exit发生的时候5个连接同时终止 会发送5个FIN，他们会使服务器发送5个SIGCHILD，正是这同一个信号多个实例递交造成了我们将要看到的问题，如果我们继续按照上面那么写，只会关闭掉一个子进程，其他僵尸进程会依然存在，使用wait并不足以防止僵尸进程，5个信号同时发生，信号处理函数只运行一次，留下四个僵尸进程，正确的处理办法是使用waitpid，在一个循环内调用waitpid，来获取所有子进程的终止状态。我们必须要设置WNOHANG选项，他告知waitpid在再有尚未终止的子进程的时候不要阻塞。

	void sig_child(int signo)
	{
		pid_t pid;
		int stat;
		while((pid = waitpid(-1,&stat,WNOHANG)) > 0)
			printf("child %d terminated\n",pid);
		return;
	}

本章节的目的是为了处理三种情况：

1.fork子进程时候捕捉SIGCHILD

2.捕获信号时候，必须处理终端的系统调用

3.信号处理进程必须正确编写，使用waitpid防止出现僵尸进程

####11.accept返回前终止连接（accept完成前客户经FIN信号发来了）

TCP建立完成三次握手之后，TCP客户端发送一个复位命令RST，在服务端看来，就在该连接已经由TCP排队，等着服务器进程调用accept的时候RST到达，稍后服务器进程调用accentpt

在这种情况下errno会返回一个EPROTO，但是由于其他错误也会产生这个错误值，所以更改为ECONNABORTED

总结：
 意味着在accept 下面还要做错误码是否为ECONNABORTED的判断，如果是continue跳过继续执行


####12.服务器进程终止

当服务器进程发生崩溃的时候我们看一下将会发生什么：

1)我们在同一个主机上启动服务器和客户端，并且在客户上输入一个文本，验证一切正常

2）找到服务器子进程的进程ID，并且执行kill杀掉他，作为进程终止处理部分的部分工作，子进程会关闭所有打开的文件描述符，然后像客户端发送一个FIN，而TCP响应一个ACK这就是TCP终止工作的前半部分

3)SIGCHILD信号发给父进程并且得到正确处理

4）客户端没有任何特殊，接收到一个FIN并且返回一个ACK，但是问题在于客户端阻塞在fgets处，等待从终端接收一行文本

5）如果我们在客户上键入一行文本，客户端把数据发送给服务端。TCP允许这么做，因为TCP接收到FIN并没有告诉客户端服务端进程已经终止

当服务器接收到请求后，由于之前的进程已经关闭会返回一个RST复位消息

6)但是客户端看不到RST会在writen后立即进行readline，并且由于第二部分的FIN，readline返回0提示错误信息

7）客户进程关闭所有的套接字被关闭

在以后的章节中可以用select和poll进行解决

####13.SIGPIPE信号

如果客户端不去理会readline返回的错误，反而写入更多的数据到服务器上，会发生什么呢，当服务器对一个已经收到复位的套接字进行写操作，内核会对进程发送一个SIGPIPE错误信号，该信号默认是终止进程，因此进程必须终止

不论该进程是否捕获该信号，还是简单的忽略，写操作都将返回EPIPE错误

####14.服务器主机崩溃

服务器主机崩溃都会发生什么？

1）如果服务器崩溃，已有的网络连接不会发出任何东西

2）TCP还是会向服务器发送内容然后readline阻塞，等待回射

3）如果我们使用TCPDUMP观察我们就会发现，客户持续发送数据分节，以期望从服务器获得ACK，重传数据分节12次，大约需要等待9分钟才会放弃重传，当TCP终于放弃重传的时候（假设服务器在这段时间内没有重启），内核会返给客户进程一个错误。既然阻塞在readline上，readline将返回错误。假设服务器主机已经崩溃，从数据分节上根本没有响应，那么返回的错误为ERIMEDOUT，如果是某个中间路由器欧安段服务器主机不可达，会相应一个"destination unreachable"的ICMP消息，那么返回的错误是EHOSTUNREACH或者ENETUNREACH

虽然客户都会发现服务器的崩溃或者不可达，但是我们需要等待9分钟

如果我们不发送数据依然想知道主机崩溃，我们可以采用SO_KEEPALIVE套接字选项

####15.服务器主机崩溃后重启

服务器主机崩溃后重启，会再次收到客户端的数据分节，由于服务器丢失了崩溃前的所有TCP信息，因此会对客户端发送RST，客户端阻塞在readline，会返回一个ECONNRESET错误，如果客户端不给主机发送信息，那么就不会直到服务器的崩溃，当然我们也可以使用SO_KEEPALIBE这个套接字选项来解决

####16.服务器主机关机

UNIX关机的时候会给所有进程发送SIGTERM信号，等待一段时间（5到20秒），会给仍然在运行的进程发送SIGKILL信号，这样给所有进程一个关闭时间，如果我们不捕获SIGTERM，那么进程将由SIGKILL关闭，然后所有的文件描述符就会关闭，然后发生向12部分一样的过程，所以客户端必须使用select或者poll来检测

####17.TCP程序小例子

getsockname获取的是服务器套接字地址以及端口

getpeername获取的是客户端的

####总结:

我们的客户服务器程序第一版大约150行，不过由很多细节我们需要关注，第一个问题就是僵尸进程的产生，我们通过捕获SIGCHILD信号加以处理，注意处理必须使用waitpid而不是早期的wait，因为unix信号是不排队的。这一点促成了我们了解linux信号的一些细节

另外我们遇到的一些问题就是服务器进程终止的时候，客户进程没有被告知。我们看到TCP服务器确实已经告知了但是由于阻塞，客户并未收到，select和poll可以解决这一个问题

我们还看到服务器发生崩溃，客户端不发送数据无法直到服务器的情况，我们可以使用SO_KEEPALIVE这个套接字选项

我们还看到服务器发送文本时不会出现问题，但是发送二进制数据时将会出现新的问题

