#第十四章高级IO函数

####14.1 概述

本章我们笼统的归为“高级IO”的各个函数和技术。首先是在IO操作上设置超时，然后是read 和 write 的几个变体：recv和send允许通过第四个参数从进程传递到内核；readv和writev允许指定往其中输入数据或从其中输出数据的缓冲区向量；recvmsg和sendmsg结合了其他IO函数的所有特性，并具备接收和发送辅助数据的新能力。

####14.2套接字超时

在涉及到IO超时操作的常用解决方案有以下三种：

（1）调用alarm，它在指定超时期满产生SIGALRM的信号。这个方法涉及到信号处理，而信号处理在不同实现上有差异，而且有可能干扰进程中alarm调用。

（2）在select 中阻塞等待IO，直接代替read和write的调用。

（3）使用比较新的SO_RCVTIMEO和SO_SNDTIMEO套接字选项。但是并非所有的实现都支持这两个套接字选项。

######14.2.1 使用SIGALRM为connect设置超时

1.使用SIGALRM建立一个信号处理函数。现有信号处理函数得以保存，以便在本函数结束的时候恢复他们。

2.设置报警，把本进程的报警时钟设置成为调用者指定的描述。如果此前已经给本进程设置过报警时钟，那么alarm的返回值是这个报警时钟的当前剩余秒数，否则返回值为0.若是前一种情况，我们还显示一个警告信息，因为推翻了之前设置的警报时钟。

3.调用connect函数

如果被系统中断（返回ENTER错误），把errno改为ETIMEOUT，同时关闭调套接字，以防止三路握手继续进行。

4.关闭alarm并恢复信号处理函数

通过以0为参数值调用alarm关闭本进程的报警时钟，同时恢复原来的信号处理函数。

5.处理SIGALRM函数

信号处理函数只是简单的返回。如果我们设想本return 语句将中断进程主控制流中那个未决的connect调用，使得她返回一个EINTR错误。回顾我们的signal函数。

我们使用这个方法可以减少connect的超时时间，但是无法减少内核中connect的时限，connect的默认时间是75秒。

利用这一到点我们使用connect的可中断能力，使用它们能够再内核超时发生之前发生EINTR中断返回。

例子：

	#include "unp.h"

	static void connect_alarm(int);

	int connect_timeo(int sockfd,const SA *saptr,socklen_t salen,int nsec)
	{
		Sigfunc *sigfunc;
		int n;
		
		sigfunc = signal(SIGALRM,connect_alarm);

		if(alarm(nsec) != 0)
			err_msg("connect_timeo:alarm was already set");

		if((n = connect(sockfd,saptr,salen)) < 0)
		{
			close(sockfd);

			if(errno == EINTR)
				errno = ETIMEDOUT;
		}

		alarm(0);//清空时钟

		signal(SIGALRM,sigfunc);

		return n;
	}

	static void connect_alarm(int signo)
	{
		return;
	}


######14.2.2 使用SIGALRM为recvfrom设置超时

	#include "unp.h"

	static void sig_alrm(int);

	void dg_cli(FILE* fp,int sockfd,const SA *pservaddr,socklen_t servlen)
	{
		int n;
		char sendline[MAXLINE],recvline[MAXLINE+1];

		signal(SIGALRM,sig_alrm);

		while(fgets(sendline,MAXLINE,fp) != NULL)
		{
			sendto(sockfd,sendline,strlen(sendline),0,pservaddr,servlen);

			alarm(5);

			if((n = recvfrom(sockfd,recvline,MAXLINE,0,NULL,NULL))<0){
				if(errno == EINTR)
				{
					fprintf(stderr,"socket timeout\n");
				}else{
					err_sys("recvfrom error");
				}
			}else{
				alarm(0);
				recvline[n] = 0;
				fputs(recvline,stdout);
			}
		}
	}

	static void sig_alrm(int signo)
	{
		return;
	}

######14.2.3 使用select为recvfrom设置超时

	#include "unp.h"

	int readable_timeo(int fd,int sec)
	{
		fd_set rset;
		struct timeval tv;

		FD_ZERO(&rset);

		FD_SET(fd,&rset);

		tv.tv_sec = sec;
		tv.tv_usec = 0;

		return select(fd+1,&rset,NULL,NULL,&tv);
	}

准备select的参数

在读描述符集中打开与调用者给定描述符对应的位。把调用者给定的等待描述设置在一个timeval结构中。

阻塞在select中

select 等待描述符变为可读，或者发生超时。版函数的返回值就是select的返回值，出错时为-1，超时发生时为0，否则返回的正值给出已经晶须描述符的数目。

使用例子：

	#include "common.h"

	int readable_timeo(int fd, int sec)
	{
		fd_set rset;
	
		struct timeval tv;
	
		FD_ZERO(&rset);
	
		FD_SET(fd, &rset);
	
		tv.tv_sec = sec;
		tv.tv_usec = 0;
	
		return select(fd+1,&rset,NULL,NULL,&tv);
	}
	
	int main()
	{
		int sockfd;
		struct sockaddr_in serveraddr, cliaddr;
		int select_result;
		int n;
		socklen_t clilen;
		char recvline[MAXLINE];
		char sendline[MAXLINE];
		//create udp socket
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = inet_addr(LOCAL);
		serveraddr.sin_port = htons(SERVPORT);
	
		bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	
		for (;;)
		{
			clilen = sizeof(cliaddr);
			if ((select_result = readable_timeo(sockfd, 10)) <= 0) {
				fprintf(stderr,"socket timeout\n");
			}
			else {
				n = recvfrom(sockfd, sendline, MAXLINE, 0, (struct sockaddr *)&cliaddr, &clilen);
				if (n < 0)
				{
					if (errno == EINTR) {
						continue;
					}
					else {
						printf("recv error\n");
					}
				}
			}
	
	
		}
	}

######14.2.4 使用SO_RCVTIMEO套接字选项为recvfrom设置超时

最后一个例子展示了SO_RCVTIMEO套接字如何设置超时的。本选项一旦设置到某个描述符，其将生效于该套接字的所有读操作。

	#include "unp.h"

	void dg_cli(FILE* fp,int sockfd,const SA *pservaddr,socklen_t servlen)
	{
		int n;
		char sendline[MAXLINE],recvline[MAXLINE+1];

		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

		while((fgets(sendline,MAXLINE,fp)) != NULL)
		{
			sendto(sockfd,sendline,strlen(sendline),0,pservaddr,servlen);

			n = recvfrom(sockfd,recvline,MAXLINE,0,NULL,NULL);

			if(n < 0)
			{
				if(errno == EWOULDBLOCK)
				{
					fprintf(stderr,"socket timeout\n");
					continue;
				}else{
					printf("recvfrom error\n");
				}
			}

			recvline[n] = 0;
			fputs(recvline,stdout);
		}
	}

14.3 recv 和 send 函数

这两个函数类似read 和 write

不过需要一个额外的参数

	#include <sys/socket.h>

	ssize_t recv(int sockfd,void* buff,size_t nbytes,int flags);

	ssize_t send(int sockfd,void* buff,size_t nbytes,int flags);

flags参数可以使用逻辑运算

	MSG_DONTROUTE 绕过路由表查找  （send）

	MSG_DONTWAIT	仅本操作非阻塞 （recv send）

	MSG_OOB		发送或接收外带数据		（recv send）

	MSG_PEEK	快看外来消息	（recv）

	MSG_WAITALL	等待所有外带数据	（recv）

MSG_DONTROUTE

本标志告知内核的目的主机在某个直接连接的本地网络上，因而无需在路由表中查找。我们已经随着SO_DONTROUTE提供了本特性的额外信息。这个既可以使用MSG_DONTROUTE标志针对单个输出操作开启，也可以使用SO_DONTROUTE套接字选项针对给某个给定套接字上的所有输出操作开启。

MSG_DONTWAIT 

本标志无需打开相应的套接字的非阻塞标志的前提下，把单个IO操作临时指定为非阻塞，接着执行IO阻塞，然后关闭非阻塞标志。我们将在16章中介绍非阻塞IO如何打开或关闭某个套接字上所有IO操作的非阻塞标志。

MSG_OOB

对于send 本标志就说明要发送外带数据。对于recv表明这个数据读取的是带外数据不是普通数据。

MSG_PEEK

本标志使用于recv和recvfrom，它允许我们查看已可读取的数据，而系统不再recv和recvfrom后丢弃这些数据。

MSG_WAITALL

它告诉内核不要再尚未读入请求数目的字节之前返回一个读操作。如果设置了我们可以忽略readn。

####14.4 readv 和 writev

readv 和 writev允许单个系统读入或者写出多个缓冲区，这些操作被称为分散读集中写

	#include <sys/uio.h>
	ssize_t readv(int filedes,const struct iovec*iov,int iovcnl);

	ssize_t writev(int filedes,const struct iovec*iov,int iovcnl);

	struct iovec{
		void *iov_base;
		size_t lov_len;
	}

iovec结构体数目最小是16，最大是1024

####14.5 recvmsg 和 sendmsg
	
	#include <sys/socket.h>

	ssize_t recvmsg(int sockfd,struct msghdr *msg,int flags);

	ssize_t sendmsg(int sockfd,struct msghdr *msg,int flags);

msghdr结构体:

	struct msghdr{
		void	*msg_name;
		socklen_t msg_namelen;
		struct iovec *msg_iov;
		int msg_iovlen;
		void *msg_control;
		socklen_t msg_controllen;
		int msg_flags;
	} 

msg_name和msg_namelen类似于recvfrom 和 sendto的第5和第6个参数，msg_name 指向一个套接字地址结构，msg_namelen是一个值参数，值结果参数

msg_iov和msg_iovlen这两个成员指定输入或输出缓冲区数组，类似于readv和writev的第二个和第三个参数。msg_control和msg_controllen指定的是辅助数据的位置和大小（值结果参数）

对于recvmsg 和sendmsg，我们必须要区分它的两个标志变量，一个是传递值的flags参数，另一个是所传递msghdr的msg_flags成员，它传递的是引用，因为传递给函数的是结构体地址。

recvmsg使用msg_flags成员。recvmsg被调用的时候，flags参数被复制到msg_flags成员，并且由内核使用其值驱动接收的处理过程。内核还依据recvmsg的结果更新msg_flags的成员值。

sendmsg则忽略msg_flags成员，因为他直接使用flags参数驱动发送处理过程。那就意味着如果某个sendmsg调用中设置MSG_DONTWAIT标志，那就把flags参数设置为该值，把msg_flags成员设置为该值并不起作用。

####14.6辅助数据

辅助数据可以通过recvmsg和sendmsg这两个函数，使用msghdr结构中的msg_control和msg_controller这两个成员发送和接收，注意unix域中SOL_SOCKET的类型SCM_RIGHTS是发送接收描述符，SCM_CREDS是发送接收用户凭证

辅助函数之中的几个常用的宏：

	#include <sys/socket.h>
	#include <sys/param.h>

	//返回第一个cmsghdr结构的指针，如果没有辅助数据那么就返回NULL
	struct cmsghdr *CMSG_FIRSTHDR(struct msghdr *mhdrptr);

	//返回：指向下一个cmsghdr结构的指针,若不再有辅助数据则对象返回NULL
	struct cmsghdr *CMSG_NXTHDR(struct msghdr *mhdrptr,struct cmsg);

	//返回：给定数据量下存放到cmsg_len中的值
	unsigned int CMSG_LEN(unsigned int length);

	//返回:给定数据量下一个辅助数据对象总的大小
	unsigned int CMSG_SPACE(unsigned int length);

	//返回指向cmsghdr结构体的第一个字节的指针
	unsigned char *CMSG_DATA(struct cmsghdr *cmsgptr); 

	

####14.7排队的数据量

查看排队数据有以下方法：

1）如果熟悉已经排队数据量的目的在于避免读操作阻塞在内核可以使用非阻塞IO

2）设置MSG_PEEK标志 

3）使用ioctl


####14.8套接字和标注IO

fdopen 可以创建一个套接字IO流，fileno可以把IO流转化称为描述符。

TCP和UDP是全双工的，只要用r+类型打开流即可,r+意味着读写。我们必须在调用一个输出函数之后插入一个fflush,fseek,fsetpos或者rewind调用才能接着调用一个输入函数。类似的调用一个输入函数也必须插入一个fseek或fsetops或rewind调用一个输出函数，除非遇到一个EOF。fseek、fsetpos和rewind问题是都调用了lseek，lseek用在套接字上回失败。

最简单的方法是给定套接字打开两个标准IO流，一个用于读 一个用于写

	#include "unp.h"

	void str_echo(int sockfd)
	{
		char line[MAXLINE];

		FILE *fpin,*fpout;

		fpin = fdopen(sockfd,"r");

		fpout = fdopen(sockfd,"w");

		while(fgets(line,MAXLINE,fpin) != NULL)
		{
			fputs(line,fpout);
		}
	}

标准IO总是存在三种缓冲

1）完全缓冲：只有缓冲区满才会出现缓冲

2）行缓冲：碰到一个换行符，调用fflush，或进程自己exit

3）不缓冲:每次IO输出函数都发生IO

unix 标准IO库出现下面的规则

标准错误输出总是不缓冲

标准输入和输出完全缓冲

所有其他IO流都是完全缓冲

这些方式都存在问题最好的方法是使用套接字的标准IO库。

####14.9高阶轮询 

现在都是epoll 了 不做学习了！！

